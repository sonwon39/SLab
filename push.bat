@echo off
set /p MSG=Commit message:
git add --all && git commit -m "%MSG%" && git push