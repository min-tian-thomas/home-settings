wsgi_app = "main:gunicorn_app"
host = "192.168.0.1"
port = 80
workers = 2
proc_name = "dvdrental_service"
worker_class = "gevent"
max_requests = 5000
graceful_timeout = 3
