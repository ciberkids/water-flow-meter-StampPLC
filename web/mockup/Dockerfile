FROM node:20-alpine

WORKDIR /app

# Install dependencies using the lockfile for reproducibility
COPY package.json package-lock.json ./
RUN npm ci

# Copy the application source
COPY . .

EXPOSE 5173

# Start Vite dev server and bind to all interfaces for Docker networking
CMD ["npm", "run", "dev", "--", "--host", "0.0.0.0", "--port", "5173"]
