FROM nginx:1.25-alpine

# Copiar archivos estáticos al directorio de nginx
COPY index.html /usr/share/nginx/html/index.html
COPY style.css  /usr/share/nginx/html/style.css
COPY app.js     /usr/share/nginx/html/app.js

# Reemplazar la config por defecto de nginx
COPY nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]