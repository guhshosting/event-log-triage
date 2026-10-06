// Independent JSON parsing and CLI integration checks; no network or shell.
const {spawnSync} = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
const dir = path.resolve(process.argv[2] || 'build-release');
const exe = path.join(dir, 'event-log-triage.exe');
function run(file, args, expected = 0) {
  const r = spawnSync(file, args, {encoding: 'utf8', maxBuffer: 128 * 1024 * 1024, timeout: 60000});
  assert.ifError(r.error);
  assert.equal(r.status, expected, r.stderr);
  return r.stdout;
}
function verify(report) {
  assert.equal(report.schema_version, '1.0');
  assert.equal(report.application_version, '1.0.0');
  for (const key of ['query','summary','event_groups','events','correlations','warnings','limitations']) assert.ok(key in report);
  assert.equal(report.summary.retained_events, report.events.length);
  assert.ok(report.events.length <= 10000);
  const start = Date.parse(report.query.start), end = Date.parse(report.query.end);
  for (const e of report.events) {
    if (Number.isFinite(start)) assert.ok(Date.parse(e.timestamp) >= start && Date.parse(e.timestamp) <= end);
    assert.ok(Number.isInteger(e.event_id));
    assert.equal(typeof e.record_id, 'string');
  }
  for (const g of report.event_groups) {
    assert.equal(g.count, g.event_indices.length);
    for (const i of g.event_indices) {
      const e = report.events[i]; assert.ok(e);
      assert.equal(e.provider, g.provider); assert.equal(e.channel,g.channel); assert.equal(e.event_id,g.event_id); assert.equal(e.level,g.level);
    }
  }
  for (const c of report.correlations) {
    assert.equal(c.causality_established, false);
    assert.ok(c.event_indices.length >= 2);
    for (const i of c.event_indices) assert.ok(report.events[i]);
    const times = c.event_indices.map(i => Date.parse(report.events[i].timestamp));
    assert.ok(Math.max(...times) - Math.min(...times) <= 120000);
  }
}
fs.mkdirSync('artifacts', {recursive:true});
const report = JSON.parse(run(exe,['--since','7d','--json'])); verify(report);
fs.writeFileSync('artifacts/real-report.json',JSON.stringify(report,null,2));
const fixture = JSON.parse(run(path.join(dir,'triage-tests.exe'),['--fixture-json'])); verify(fixture);
assert.ok(fixture.events.some(e=>e.provider==='テスト'));
assert.ok(fixture.events.some(e=>e.message.includes('\x1b')));
fs.writeFileSync('artifacts/unicode-fixture.json',JSON.stringify(fixture,null,2));
const missing = JSON.parse(run(exe,['--log','event-log-triage-nonexistent-channel-947208','--json'],3)); verify(missing);
assert.equal(missing.query.logs[0].status,'unavailable');
for (const args of [['--since','32d'],['--limit','101'],['--log'],['--level','0'],['--wat'],['--json','--json']]) run(exe,args,2);
assert.match(run(exe,['--version']), /1\.0\.0/);
assert.match(run(exe,['--help']), /Usage:/);
console.log(`Independent JSON/CLI checks passed: ${report.events.length} real events, ${report.events.filter(e=>e.message_status==='formatted').length} messages formatted, ${report.events.filter(e=>e.message_status!=='formatted').length} fallbacks.`);
