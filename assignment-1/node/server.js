const express = require('express');
const os = require('os');

const app = express();
const port = process.env.PORT || 8080;
const greeting = process.env.APP_GREETING || 'Hello from Express in Docker!';

app.get('/', (req, res) => {
  res.json({
    message: greeting,
    hostname: process.env.HOSTNAME || os.hostname(),
    client: req.ip,
    path: req.path,
  });
});

app.listen(port, '0.0.0.0', () => {
  console.log(`Server listening on http://0.0.0.0:${port}`);
});

