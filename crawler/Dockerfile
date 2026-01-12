FROM ghcr.io/astral-sh/uv:python3.12-alpine

WORKDIR /app

COPY pyproject.toml uv.lock ./

ENV UV_NO_DEV=1

RUN uv sync --locked

COPY . .

ENTRYPOINT [ "uv", "run" ]
CMD [ "main.py", "config.yaml" ]