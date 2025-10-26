import os
from flask import Flask, jsonify, request

app = Flask(__name__)


@app.get("/")
def root():
    return jsonify(
        {
            "message": os.getenv("APP_GREETING", "Hello from Flask in Docker!"),
            "hostname": os.getenv("HOSTNAME", "unknown"),
            "client": request.remote_addr,
            "path": request.path,
        }
    )


if __name__ == "__main__":
    # For local dev only; Docker uses the Flask CLI in the Dockerfile
    app.run(host="0.0.0.0", port=int(os.getenv("PORT", 5000)))

