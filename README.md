The `RandomFields` package is not available on CRAN anymore. It currently fails to compile on certain configurations due to the `-Wformat-security` flag. There are a couple of references online for this issue, see, e.g., [this](https://github.com/RcppCore/Rcpp/issues/1287).

This package implements a straightforward fix.
