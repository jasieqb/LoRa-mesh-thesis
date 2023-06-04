FROM python:3.10-slim-buster

WORKDIR /app
# Install Python dependencies
COPY ./requirements.txt /app/requirements.txt
RUN pip install -r /app/requirements.txt

# Copy the rest of the application
COPY . /app

# Run the application
CMD ["python", "connector.py"]
