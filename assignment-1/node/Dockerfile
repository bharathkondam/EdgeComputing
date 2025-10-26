# Node Express app, exposed on port 8080
FROM node:20-alpine

ENV NODE_ENV=production \
    PORT=8080 \
    APP_GREETING="Hello from Express in Docker!"

WORKDIR /app

COPY package.json ./
RUN npm install --omit=dev --no-audit --no-fund

COPY . .

EXPOSE 8080

CMD ["node", "server.js"]

