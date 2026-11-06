import { defineConfig } from 'vitest/config';
// Explicit file list avoids root globalSetup copying tiles for pure doc contracts.
// These are the existing product tests, not replicas of their assertions.
export default defineConfig({
  test: {
    include: [
      'packages/desktop/src/packaging.test.ts',
      'packages/core/src/toolchain.test.ts',
    ],
  },
});
