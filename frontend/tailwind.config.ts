import type { Config } from 'tailwindcss';

// SwapCycle design tokens. `colors` replaces Tailwind's default palette so
// only these colours can be used anywhere in the app.
export default {
  content: ['./index.html', './src/**/*.{ts,tsx}'],
  theme: {
    colors: {
      transparent: 'transparent',
      current: 'currentColor',
      white: '#FFFFFF',
      navy: {
        DEFAULT: '#1B2A4A', // primary, headings, text
        deep: '#111C33', // hover/pressed, dark screens
      },
      blush: '#FFF7F9', // page background
      pink: {
        light: '#FCE4EC', // cards and surfaces
      },
      rose: {
        DEFAULT: '#F4A6BD', // accents and highlights
        deep: '#D9708F', // primary action buttons, links
      },
      soft: '#EBCDD7', // soft borders
      slate: '#5B6785', // muted text
      success: '#3F8F6B',
      warning: '#D99A2B',
      danger: '#C94A5E',
    },
    fontFamily: {
      display: ['"Playfair Display"', 'Georgia', 'serif'],
      sans: ['Poppins', 'system-ui', 'sans-serif'],
    },
    extend: {
      borderRadius: {
        card: '16px',
      },
    },
  },
  plugins: [],
} satisfies Config;
