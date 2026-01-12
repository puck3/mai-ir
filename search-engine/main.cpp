#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "index.hpp"

inline std::string truncate_string(const std::string& str, size_t max_length) {
  if (str.length() <= max_length) {
    return str;
  }
  return str.substr(0, max_length - 3) + "...";
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <corpus.txt>\n";
    return 1;
  }

  InvertedIndex idx;
  HashMap<std::string> doc_titles;
  HashMap<std::string> doc_sources;

  std::ifstream fin(argv[1]);
  if (!fin) {
    std::cerr << "Cannot open file\n";
    return 1;
  }

  int doc_count = 0;
  std::string url, source, title, text;

  while (std::getline(fin, url)) {
    if (url.empty()) {
      continue;
    }

    if (!std::getline(fin, source)) break;
    if (!std::getline(fin, title)) break;
    if (!std::getline(fin, text)) break;

    doc_titles.insert(url, title);
    doc_sources.insert(url, source);

    std::string full_text = title + " " + text;
    idx.index_document(url, full_text);

    doc_count++;
    if (doc_count % 5000 == 0) {
      std::cerr << "Indexed " << doc_count << " documents\n";
    }
  }

  std::cerr << "Total indexed: " << doc_count << " documents\n";

  idx.print_index_stats();

  idx.write_zipf_statistics("../data/zipf_statistics.txt");

  std::cout
      << "Index ready. Queries: слово | слово AND слово | слово OR слово\n";
  std::cout << "Examples: fifa | FIFA AND евро | футбол OR спорт\n\n";

  while (true) {
    std::cout << "> ";
    std::string query;
    if (!std::getline(std::cin, query)) break;

    while (!query.empty() && query[0] == ' ') query.erase(0, 1);
    while (!query.empty() && query.back() == ' ') query.pop_back();

    if (query.empty()) continue;

    std::istringstream ss(query);
    std::string a, b, op;
    ss >> a >> op >> b;

    std::vector<DocID> res;
    if (op.empty()) {
      res = idx.search_term(a);
    } else if (op == "AND" && !b.empty()) {
      res = idx.search_and(a, b);
    } else if (op == "OR" && !b.empty()) {
      res = idx.search_or(a, b);
    } else {
      std::cout
          << "Invalid query. Use: слово | слово AND слово | слово OR слово\n";
      continue;
    }

    if (res.empty()) {
      std::cout << "No documents found for query: \"" << query << "\"\n";
    } else {
      std::cout << "\nFound " << res.size() << " document(s) for query: \""
                << query << "\"\n\n";

      size_t show_count = (res.size() < 5) ? res.size() : 5;
      for (size_t i = 0; i < show_count; i++) {
        const std::string& url = res[i];
        std::string* title_ptr = doc_titles.find(url);
        std::string* source_ptr = doc_sources.find(url);

        std::cout << (i + 1) << ". ";
        if (title_ptr) {
          std::cout << *title_ptr;
        } else {
          std::cout << "(No title)";
        }
        std::cout << "\n   URL: " << url << "\n";
        if (source_ptr) {
          std::cout << "   Source: " << *source_ptr << "\n";
        }
        std::cout << "\n";
      }

      if (res.size() > 5) {
        std::cout << "... and " << (res.size() - 5) << " more document(s)\n";
      }
      std::cout << "----------------------------------------\n";
    }
  }

  return 0;
}