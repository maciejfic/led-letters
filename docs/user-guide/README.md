# User guide

`led-letters-user-guide.pdf` is built from `user-guide.html` (A4, print styles inline). Screenshots
come from `docs/images/`, taken from the app running in `tools/mock_server.py` at phone size.

Rebuild the PDF with Chrome or Chromium:

```bash
cd docs/user-guide
"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" --headless=new --no-pdf-header-footer \
  --print-to-pdf=led-letters-user-guide.pdf "file://$PWD/user-guide.html"
```

On Linux use `chromium --headless=new ...` with the same arguments. Section 3 (soldering and wiring)
is a placeholder to be filled in.
