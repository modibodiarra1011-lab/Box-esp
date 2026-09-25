#!/usr/bin/env python3
"""Check that the embedded UIs reference routes registered by their servers."""
from __future__ import annotations
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

def routes_from_master(text: str) -> set[str]:
    out = set(re.findall(r'REG\("([^" ]+)"', text))
    out.update(re.findall(r'httpd_uri_t\s+\w+\s*=\s*\{\.uri\s*=\s*"([^"]+)"', text))
    return out

def routes_from_worker(text: str) -> set[str]:
    return set(re.findall(r'server\.on\("([^" ]+)",\s*HTTP_(?:GET|POST|PUT|DELETE)', text))

master_ui = (ROOT / 'firmware/master/main/web_ui.cpp').read_text(encoding='utf-8')
master_server = (ROOT / 'firmware/master/main/web_server.c').read_text(encoding='utf-8')
worker_ui = (ROOT / 'WORKER/Arduino/worker.ino').read_text(encoding='utf-8')
worker_server = worker_ui

master_expected = {
    '/api/state','/api/jobs','/api/logs','/api/job','/api/job/cancel','/api/jobs/cancel-all',
    '/api/sd/list','/api/worker/flash','/api/worker/reboot','/api/update/check','/api/update/approve',
    '/api/agent/chat'
}
worker_expected = {'/api/info','/api/job','/api/cancel','/api/scan','/api/reboot'}

m_routes = routes_from_master(master_server)
w_routes = routes_from_worker(worker_server)
missing_m = sorted(r for r in master_expected if r not in m_routes)
missing_w = sorted(r for r in worker_expected if r not in w_routes)
if missing_m: raise SystemExit(f'FAIL master routes: {missing_m}')
if missing_w: raise SystemExit(f'FAIL worker routes: {missing_w}')
for route in master_expected:
    if route not in master_ui: raise SystemExit(f'FAIL master UI missing route string: {route}')
for route in worker_expected:
    if route not in worker_ui: raise SystemExit(f'FAIL worker UI missing route string: {route}')

if '/ws' not in master_server: raise SystemExit('FAIL master WebSocket route missing')
print(f'Web contract checks: PASS (master={len(master_expected)}, worker={len(worker_expected)})')
