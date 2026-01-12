import time
import yaml
import hashlib
import threading
import requests
from urllib.parse import urlparse, urlunparse
from pymongo import MongoClient, errors
from bs4 import BeautifulSoup
import logging


logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] [%(threadName)s] %(message)s",
)


def load_config(path: str) -> dict:
    with open(path, "r") as f:
        return yaml.safe_load(f)


def normalize_url(url: str) -> str:
    parsed = urlparse(url)
    return urlunparse(
        (
            parsed.scheme,
            parsed.netloc.lower(),
            parsed.path.rstrip("/"),
            "",
            "",
            "",
        )
    )


def md5(text: str) -> str:
    return hashlib.md5(text.encode("utf-8")).hexdigest()


def fetch(url: str, headers: dict) -> str | None:
    try:
        r = requests.get(url, headers=headers, timeout=15)
    except requests.RequestException as e:
        logging.warning(f"Request failed {url}: {e}")
        return None

    if r.status_code != 200:
        return None
    try:
        text = r.content.decode("utf-8", errors="replace")
    except Exception as e:
        logging.warning(f"Decode failed {url}: {e}")
        return None

    return text


def parse_sitemap(xml_text: str) -> list[str]:
    soup = BeautifulSoup(xml_text, "xml")
    return [loc.text for loc in soup.find_all("loc")]


def extract_main_content(
    html: str, container_class: str | None
) -> str:
    if not container_class:
        return html

    soup = BeautifulSoup(html, "lxml")

    container = soup.find(class_=container_class)
    if not container:
        logging.warning(
            f"Content container '{container_class}' not found, saving full html"
        )
        return html

    return str(container)


def crawl_source(
    source: dict,
    collection,
    headers: dict,
    stop_event: threading.Event,
):
    source_name = source["name"]
    logging.info(f"[{source_name}] crawler started")

    delay = source["request_delay_sec"]
    patterns = source["include_patterns"]
    container_class = source.get("content_container_class")

    index_xml = fetch(source["index_url"], headers)
    if not index_xml:
        logging.warning(
            f"[{source_name}] failed to fetch sitemap index"
        )
        return

    sitemap_urls = parse_sitemap(index_xml)

    sitemap_urls = [
        url
        for url in sitemap_urls
        if all(p in url for p in patterns)
    ]

    logging.info(
        f"[{source_name}] matched {len(sitemap_urls)} sitemaps"
    )

    for sitemap_url in sitemap_urls:
        if stop_event.is_set():
            logging.info(
                f"[{source_name}] shutdown requested"
            )
            return

        sitemap_xml = fetch(sitemap_url, headers)
        if not sitemap_xml:
            continue

        page_urls = parse_sitemap(sitemap_xml)
        logging.info(
            f"[{source_name}] sitemap {sitemap_url} → {len(page_urls)} urls"
        )

        for url in page_urls:
            if stop_event.is_set():
                logging.info(
                    f"[{source_name}] shutdown requested"
                )
                return

            norm_url = normalize_url(url)
            now = int(time.time())

            logging.debug(f"[{source_name}] fetching {url}")

            raw_html = fetch(url, headers)
            if not raw_html:
                logging.warning(
                    f"[{source_name}] failed to fetch {url}"
                )
                continue

            html = extract_main_content(
                raw_html, container_class
            )
            content_hash = md5(html)

            try:
                collection.update_one(
                    {"url_norm": norm_url},
                    {
                        "$setOnInsert": {
                            "url": url,
                            "url_norm": norm_url,
                            "source": source_name,
                        },
                        "$set": {
                            "html": html,
                            "content_hash": content_hash,
                            "fetched_at": now,
                            "last_checked": now,
                        },
                    },
                    upsert=True,
                )

                logging.info(f"[{source_name}] saved {url}")

            except errors.DuplicateKeyError:
                logging.debug(
                    f"[{source_name}] duplicate {url}"
                )

            time.sleep(delay)

    logging.info(f"[{source_name}] crawler finished")


def main(config_path: str):
    config = load_config(config_path)

    mongo = MongoClient(config["db"]["uri"])
    collection = mongo[config["db"]["database"]][
        config["db"]["collection"]
    ]
    collection.create_index("url_norm", unique=True)

    headers = {"User-Agent": config["logic"]["user_agent"]}

    stop_event = threading.Event()
    threads = []

    try:
        for source in config["logic"]["sources"]:
            t = threading.Thread(
                target=crawl_source,
                args=(
                    source,
                    collection,
                    headers,
                    stop_event,
                ),
                name=f"crawler-{source['name']}",
                daemon=False,
            )
            t.start()
            threads.append(t)

        for t in threads:
            t.join()

    except KeyboardInterrupt:
        logging.warning("Shutdown requested (Ctrl+C)")
        stop_event.set()

        for t in threads:
            t.join()

        logging.info("All crawlers stopped gracefully")


if __name__ == "__main__":
    import sys

    main(sys.argv[1])
