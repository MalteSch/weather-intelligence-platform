FROM node:20-bookworm

WORKDIR /app

COPY package*.json ./

RUN npm install --omit=dev

COPY server.js ./
COPY database.js ./
COPY public ./public

RUN mkdir -p /app/data

EXPOSE 3000

CMD ["npm", "start"]