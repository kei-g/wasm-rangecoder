import * as fs from 'fs'

abstract class Stream {
	protected readonly buffer: Buffer
	protected fd: number = undefined
	protected index: number = 0

	constructor(protected readonly path: fs.PathLike, protected readonly bufferSize: number) {
		this.buffer = Buffer.alloc(this.bufferSize)
	}

	close(): void {
		this.fd ?? fs.closeSync(this.fd)
	}
}

class InputStream extends Stream {
	private length: number = 0
	private offset: number = 0
	private readonly view: Uint8Array

	constructor(path: fs.PathLike, bufferSize: number = 65536) {
		super(path, bufferSize)
		this.view = new Uint8Array(this.bufferSize)
	}

	get byte(): number {
		return this.index == this.length && !this.read() ? -1 : this.view[this.index++]
	}

	private read(): boolean {
		this.fd ?? (this.fd = fs.openSync(this.path, "r"))
		const readLength = fs.readSync(this.fd, this.buffer, 0, this.bufferSize, this.offset)
		if (0 < readLength) {
			this.buffer.copy(this.view)
			this.index = 0
			this.length = readLength
			this.offset += readLength
			return true
		}
	}

	rewind(): void {
		this.index = 0
		this.length = 0
		this.offset = 0
	}

	get size(): number {
		return fs.statSync(this.path).size
	}
}

class OutputStream extends Stream {
	private readonly view: Uint8Array

	constructor(path: fs.PathLike, bufferSize: number = 65536) {
		super(path, bufferSize)
		this.view = new Uint8Array(this.buffer.buffer)
	}

	set byte(value: number) {
		if (this.index == this.bufferSize)
			this.flush()
		this.view[this.index++] = value
	}

	flush(): void {
		this.fd = this.fd ?? fs.openSync(this.path, "w")
		fs.writeSync(this.fd, this.buffer, 0, this.index)
		this.index = 0
	}
}

const frequency = new Map<number, number>()
const input = [new InputStream('alice29.txt'), new InputStream('alice29.txt.rc')]
const output = [new OutputStream('alice29.txt.rc'), new OutputStream('decompressed.txt')]

const imports: WebAssembly.Imports = {
	env: {
		memory: new WebAssembly.Memory({
			initial: 16,
		}),
		memoryBase: 0,
		table: new WebAssembly.Table({
			element: 'anyfunc',
			initial: 4,
		}),
		tableBase: 0,
		__flush: (id: number): void => output[id].flush(),
		__getbyte: (id: number): number => input[id].byte,
		__putbyte: (id: number, value: number): void => { output[id].byte = value },
		__rewind: (id: number): void => input[id].rewind(),
		getdecompressedsize: (id: number): number => input[0].size,
		getfreq: (id: number, symbol: number): number => frequency.get(symbol),
		putfreq: (id: number, symbol: number, count: number): void => { frequency.set(symbol, count) },
	}
}

type CompressFunction = (id: number) => void
type DecompressFunction = (id: number) => boolean

WebAssembly.compile(fs.readFileSync('rangecoder.wasm'))
	.then((module: WebAssembly.Module) => WebAssembly.instantiate(module, imports))
	.then((instance: WebAssembly.Instance) => {
		const compress = instance.exports.compress as CompressFunction
		const decompress = instance.exports.decompress as DecompressFunction
		compress(0)
		input[0].close()
		output[0].close()
		decompress(1)
		input[1].close()
		output[1].close()
	})
	.catch(reason => console.log(`error: ${reason}`))
