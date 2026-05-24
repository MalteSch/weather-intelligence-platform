FROM node:20-bookworm

WORKDIR /app

RUN apt-get update && apt-get install -y \
    python3 \
    make \
    g++ \
  && rm -rf /var/lib/apt/lists/*

COPY package*.json ./

RUN npm install --omit=dev --build-from-source=sqlite3

COPY server.js ./
COPY database.js ./
COPY public ./public

RUN mkdir -p /app/data

EXPOSE 3000

CMD ["npm", "start"]