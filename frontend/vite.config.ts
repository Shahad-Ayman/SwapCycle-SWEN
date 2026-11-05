import react from '@vitejs/plugin-react';
import { loadEnv } from 'vite';
import { defineConfig } from 'vitest/config';

// Headers for the dev and preview servers. The production web server must
// send the same (plus a Content-Security-Policy, which would block Vite's dev
// tooling, so it is not set here).
const securityHeaders = {
  'X-Content-Type-Options': 'nosniff',
  'X-Frame-Options': 'DENY',
  'Referrer-Policy': 'strict-origin-when-cross-origin',
};

export default defineConfig(({ mode }) => {
  // Reads VITE_* values from the repository root .env (see .env.example).
  const env = loadEnv(mode, '..', 'VITE_');
  // Same-origin /api, so the session cookie works without CORS.
  const proxy = {
    '/api': { target: env.VITE_API_PROXY_TARGET || 'http://localhost:8080', changeOrigin: false },
  };

  return {
    plugins: [react()],
    server: { port: 5173, strictPort: true, proxy, headers: securityHeaders },
    preview: { port: 4173, strictPort: true, proxy, headers: securityHeaders },
    build: { sourcemap: false, target: 'es2020' },
    test: {
      environment: 'jsdom',
      setupFiles: ['./vitest.setup.ts'],
      css: false,
      include: ['src/**/*.test.{ts,tsx}'],
    },
  };
});
