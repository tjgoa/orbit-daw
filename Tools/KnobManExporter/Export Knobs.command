#!/bin/zsh
set -eu
cd -- "${0:A:h}"
if [[ ! -x .runtime/venv/bin/python ]]; then
  echo 'First run: setting up an isolated Python environment for the exporter.'
  python3 -m venv .runtime/venv
fi
.runtime/venv/bin/python -m pip install --disable-pip-version-check -r requirements.txt
set +e
.runtime/venv/bin/python export_knobs.py --choose --open "$@"
result=$?
echo
if [[ $result -eq 0 ]]; then
  echo 'Export finished. Your original KnobMan projects have not been modified.'
elif [[ $result -eq 130 ]]; then
  echo 'Cancelled.'
else
  echo 'Some exports failed. Read the messages above and the output manifest/gallery.'
fi
read -r '?Press Return to close this window.'
exit $result
