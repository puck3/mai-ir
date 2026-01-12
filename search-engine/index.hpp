#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "hash_map.hpp"
#include "stemming.hpp"
#include "tokenization.hpp"

typedef std::string DocID;

struct PostingList {
  std::vector<DocID> docs;
};

class InvertedIndex {
 private:
  HashMap<PostingList> index;
  HashMap<size_t> term_frequencies;
  size_t total_terms = 0;

 public:
  void index_document(DocID id, const std::string& text) {
    std::vector<std::string> tokens;
    tokenize(text, tokens);

    HashMap<bool> doc_terms;

    for (auto& t : tokens) {
      std::string term = t;
      to_lower_utf8(term);
      stem(term);

      if (!term.empty() && !doc_terms.find(term)) {
        doc_terms.insert(term, true);
        total_terms++;

        size_t* freq = term_frequencies.find(term);
        if (freq) {
          (*freq)++;
        } else {
          term_frequencies.insert(term, 1);
        }

        PostingList* pl = index.find(term);
        if (!pl) {
          PostingList new_pl;
          new_pl.docs.push_back(id);
          index.insert(term, new_pl);
        } else {
          pl->docs.push_back(id);
        }
      }
    }
  }

  std::vector<DocID> search_term(const std::string& query) {
    std::string q = query;
    to_lower_utf8(q);
    stem(q);

    PostingList* pl = index.find(q);
    if (!pl) return {};

    if (pl->docs.size() > 1) {
      HashMap<bool> seen;
      std::vector<DocID> unique_docs;
      for (const auto& doc : pl->docs) {
        if (!seen.find(doc)) {
          seen.insert(doc, true);
          unique_docs.push_back(doc);
        }
      }
      pl->docs = std::move(unique_docs);
    }

    return pl->docs;
  }

  std::vector<DocID> intersect(const std::vector<DocID>& a,
                               const std::vector<DocID>& b) {
    if (a.empty() || b.empty()) return {};

    const std::vector<DocID>& smaller = (a.size() < b.size()) ? a : b;
    const std::vector<DocID>& larger = (a.size() < b.size()) ? b : a;

    HashMap<bool> hash_table;
    for (const auto& doc : smaller) {
      hash_table.insert(doc, true);
    }

    std::vector<DocID> result;
    for (const auto& doc : larger) {
      if (hash_table.find(doc)) {
        result.push_back(doc);
      }
    }

    return result;
  }

  std::vector<DocID> unite(const std::vector<DocID>& a,
                           const std::vector<DocID>& b) {
    HashMap<bool> hash_table;
    std::vector<DocID> result;

    for (const auto& doc : a) {
      if (!hash_table.find(doc)) {
        hash_table.insert(doc, true);
        result.push_back(doc);
      }
    }

    for (const auto& doc : b) {
      if (!hash_table.find(doc)) {
        hash_table.insert(doc, true);
        result.push_back(doc);
      }
    }

    return result;
  }

  std::vector<DocID> search_and(const std::string& a, const std::string& b) {
    return intersect(search_term(a), search_term(b));
  }

  std::vector<DocID> search_or(const std::string& a, const std::string& b) {
    return unite(search_term(a), search_term(b));
  }

  void quick_sort_pairs(std::vector<std::pair<std::string, size_t>>& pairs,
                        int left, int right) {
    if (left >= right) return;

    size_t pivot = pairs[(left + right) / 2].second;
    int i = left, j = right;

    while (i <= j) {
      while (pairs[i].second > pivot) i++;
      while (pairs[j].second < pivot) j--;

      if (i <= j) {
        std::swap(pairs[i], pairs[j]);
        i++;
        j--;
      }
    }

    if (left < j) quick_sort_pairs(pairs, left, j);
    if (i < right) quick_sort_pairs(pairs, i, right);
  }

  void write_zipf_statistics(const std::string& filename) {
    std::vector<std::pair<std::string, size_t>> pairs;
    term_frequencies.get_all_pairs(pairs);

    if (!pairs.empty()) {
      quick_sort_pairs(pairs, 0, pairs.size() - 1);
    }

    std::ofstream fout(filename);
    if (!fout) {
      std::cerr << "Error opening file for writing statistics: " << filename
                << std::endl;
      return;
    }

    fout << "rank\tterm\tfrequency\n";

    for (size_t i = 0; i < pairs.size(); i++) {
      fout << (i + 1) << "\t" << pairs[i].first << "\t" << pairs[i].second
           << "\n";
    }

    fout.close();
    std::cerr << "Statistics written to " << filename << std::endl;
    std::cerr << "Total unique terms: " << pairs.size() << std::endl;
    std::cerr << "Total terms: " << total_terms << std::endl;
  }

  void print_index_stats() {
    std::vector<std::pair<std::string, size_t>> pairs;
    term_frequencies.get_all_pairs(pairs);
    std::cerr << "Index statistics:" << std::endl;
    std::cerr << "  Unique terms: " << pairs.size() << std::endl;
    std::cerr << "  Total term occurrences: " << total_terms << std::endl;

    if (!pairs.empty()) {
      quick_sort_pairs(pairs, 0, pairs.size() - 1);
      size_t top_n = (pairs.size() < 10) ? pairs.size() : 10;
      std::cerr << "  Top " << top_n << " terms:" << std::endl;
      for (size_t i = 0; i < top_n; i++) {
        std::cerr << "    " << (i + 1) << ". " << pairs[i].first << " ("
                  << pairs[i].second << " occurrences)" << std::endl;
      }
    }
  }
};