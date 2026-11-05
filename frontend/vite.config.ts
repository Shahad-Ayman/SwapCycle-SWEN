import react from '@vitejs/plugin-react';
import { loadEnv } from 'vite';
import { defineConfig } from 'vitest/config';

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, '..', 'VITE_');
  return {
    plugins: [react()],
    server: {
      port: 5173,
      // Same-origin /api in development, so the session cookie just works.
      proxy: {
        '/api': { target: env.VITE_API_PROXY_TARGET || 'http://localhost:8080', changeOrigin: false },
      },
    },
    test: {
      environment: 'jsdom',
      setupFiles: ['./vitest.setup.ts'],
      css: false,
    },
  };
});
