import assert from './assert.js';

const thisFile = import.meta.url.slice(7);   // strip "file://"


(async () => {
    const args = [
        ijjs.exepath(),
        '--abort-on-unhandled-rejection',
        ijjs.join(ijjs.dirname(thisFile), 'helpers', 'unhandled-rejection.js')
    ];
    const proc = ijjs.spawn(args, { stdout: 'ignore', stderr: 'pipe' });
    const stderr = await proc.stderr.read();
    const stderrStr = new TextDecoder().decode(stderr);
    const status = await proc.wait();
    assert.ok(stderrStr.match(/Unhandled promise rejected, aborting!/) !== null, 'dumps to stderr');
    assert.ok(status.exit_status !== 0 || status.term_signal === ijjs.signal.SIGABRT, 'process failed')
})();
