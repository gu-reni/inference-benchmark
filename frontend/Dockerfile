FROM docker.m.daocloud.io/library/node:18-alpine as builder

WORKDIR /app

COPY package*.json ./
RUN npm ci --registry=https://registry.npmmirror.com

COPY public/ ./public/
COPY src/ ./src/
COPY vue.config.js .

RUN npm run build

FROM docker.m.daocloud.io/library/nginx:alpine

COPY --from=builder /app/dist /usr/share/nginx/html

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]