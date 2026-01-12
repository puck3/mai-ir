import re
from pymongo import MongoClient
from bs4 import BeautifulSoup


MONGO_URI = "mongodb://localhost:27017"
DB_NAME = "news_crawler"
COLLECTION = "documents"

OUTPUT_FILE = "../data/corpus.txt"


def clean_text(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def extract_title_and_text(html: str):
    soup = BeautifulSoup(html, "html.parser")

    h1 = soup.find("h1")
    title = clean_text(h1.get_text()) if h1 else ""

    paragraphs = soup.find_all("p")
    text = " ".join(p.get_text() for p in paragraphs)

    return title, clean_text(text)


def main():
    client = MongoClient(MONGO_URI)
    collection = client[DB_NAME][COLLECTION]

    exported = 0

    with open(OUTPUT_FILE, "w", encoding="utf-8") as out:
        for doc in collection.find({}, no_cursor_timeout=True):
            html = doc.get("html")
            if not html:
                continue

            title, text = extract_title_and_text(html)

            if not text:
                continue

            out.write(f"{doc['url_norm']}\n")
            out.write(f"{doc.get('source', 'unknown')}\n")
            out.write(f"{title}\n")
            out.write(f"{text}\n")

            exported += 1
            if exported % 1000 == 0:
                print(f"Exported {exported} documents")

    print(f"Done. Total documents exported: {exported}")


if __name__ == "__main__":
    main()
