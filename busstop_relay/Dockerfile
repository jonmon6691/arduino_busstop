# Use official lightweight base image
FROM alpine:latest

RUN apk update && apk add --no-cache python3 py3-pip
# Set working directory
WORKDIR /app

# Copy app script into container
COPY app.sh /app/app.sh
RUN chmod +x /app/app.sh

# Copy relay.py into container
COPY relay.py /app/relay.py

# Copy key files into container
COPY --chmod=400 keys/root.crt /app/keys/root.crt
COPY --chmod=400 keys/relay.crt /app/keys/relay.crt
COPY --chmod=400 keys/relay.key /app/keys/relay.key

# Run the app script on container start
CMD ["/app/app.sh"]
