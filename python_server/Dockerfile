FROM python:3.11-slim
WORKDIR /app
RUN pip install mysql-connector-python
COPY server.py .
CMD ["python", "-u", "server.py"]
