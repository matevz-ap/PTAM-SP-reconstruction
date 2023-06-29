import os

import redis
import requests
from rq import Worker, Connection

listen = ['default']
# conn = redis.from_url("redis://redis:6379/0")
redis_url = os.getenv('REDISTOGO_URL', 'redis://localhost:6379')
conn = redis.from_url(redis_url)

if __name__ == '__main__':
    with Connection(conn):
        worker = Worker(listen)
        worker.work()