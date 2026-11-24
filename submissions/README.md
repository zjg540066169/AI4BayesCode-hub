# submissions/: pending block submissions

This folder contains block bundles awaiting review. A contributor submits a block
from the **Block upload** tab on the site's Blocks page. The submission service
commits the zipped bundle to `<block>/<block>.zip` in this folder and opens a
GitHub review issue that links to the zip file. A maintainer extracts the bundle
here, reviews and tests it, and on acceptance moves it to `../registry/`, where
it is published on the site.
