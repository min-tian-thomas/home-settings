load("@python_deps//:requirements.bzl", "requirement")

py_binary(
    name = "app",
    srcs = ["app.py"],
    deps = [
        requirement("Flask"),
        requirement("Flask-Caching"),
        requirement("redis"),
        requirement("configparser"),
    ],
)

py_binary(
    name = "gunicorn",
    srcs = ["wsgi.py"],
    deps = [
        requirement("gunicorn"),
    ],
)
