# 构建阶段
FROM node:18-alpine as builder

WORKDIR /app
COPY package*.json ./
RUN npm ci

COPY public/ ./public/
COPY src/ ./src/
COPY vue.config.js .

RUN npm run build

# 运行阶段：使用 Nginx 托管静态文件
FROM nginx:alpine

COPY --from=builder /app/dist /usr/share/nginx/html
EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]