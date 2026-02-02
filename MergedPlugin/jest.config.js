module.exports = {
  testEnvironment: 'node',
  roots: ['<rootDir>'],
  moduleNameMapper: {
    '^ACAPI$': '<rootDir>/__mocks__/ACAPI.js'
  },
  testMatch: [
    '<rootDir>/__tests__/**/*.test.js',
    '<rootDir>/__tests__/**/*.spec.js'
  ],
  collectCoverageFrom: [
    '**/*.{js,jsx}',
    '!**/node_modules/**',
    '!**/vendor/**'
  ]
};