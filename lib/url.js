'use strict';

const puny = require('punycode');
const { inspect, toUSVString: _toUSVString } = require('util');

const {
  NativeURL,
  NativeURLSearchParams,
  init,
} = require('../build/Release/binding.node');

init({
  toASCII(domain) {
    // Workaround
    if (domain === 'xn--') {
      return '';
    }

    try {
      return puny.toASCII(domain);
    } catch (e) {
      return '';
    }
  },
});

const customInspectSymbol = Symbol.for('nodejs.util.inspect.custom');
const CORE = Symbol('URL#core');
const INIT = Symbol('URL#init');
const ON_PASSIVELY_UPDATE = Symbol('URL#onPassivelyUpdate');
const SEARCH_PARAMS = Symbol('URL#searchParams');
const STRUCTED = Symbol('URL#structed');

function requiredArguments(name, length, required) {
  if (length < required) {
    const errMsg = `${name} requires at least ${required} argument${
      required === 1 ? '' : 's'
    }, but only ${length} present`;
    throw new TypeError(errMsg);
  }
}

// https://infra.spec.whatwg.org/#javascript-string-convert
const surrogateRegexp =
  /(?:[^\uD800-\uDBFF]|^)[\uDC00-\uDFFF]|[\uD800-\uDBFF](?![\uDC00-\uDFFF])/;
function toUSVString(val) {
  const str = `${val}`;
  const match = surrogateRegexp.exec(str);
  if (!match) {
    return str;
  }
  return _toUSVString(str, match.index);
}

function isIterable(o) {
  // checks for null and undefined
  if (o == null) {
    return false;
  }

  return typeof (o)[Symbol.iterator] === 'function';
}

class URLSearchParams {
  constructor(init = '') {
    if (init instanceof NativeURLSearchParams) {
      this[CORE] = init;
      return;
    }

    if (typeof init !== 'string' && isIterable(init)) {
      init = [ ...init ];
    }

    if (Array.isArray(init)) {
      for (let i = 0; i < init.length; i++) {
        const tuple = init[i];
        if (!tuple || tuple.length !== 2) {
          throw new TypeError(
            'URLSearchParams.constructor tuple array argument must only ' +
            'contain pair elements');
        }

        if (!Array.isArray(tuple)) {
          init[i] = [ toUSVString(tuple[0]), toUSVString(tuple[1]) ];
        } else {
          tuple[0] = toUSVString(tuple[0]);
          tuple[1] = toUSVString(tuple[1]);
        }
      }
    } else if (typeof init === 'object' || typeof init === 'function') {
      const array = [];
      const visited = {};
      for (const key of Object.keys(init)) {
        const replaced = toUSVString(key);
        if (visited[replaced]) {
          visited[replaced][1] = toUSVString(init[key]);
        } else {
          const temp = [ replaced, toUSVString(init[key]) ];
          visited[replaced] = temp;
          array.push(temp);
        }
      }

      init = array;
    } else {
      init = toUSVString(`${init}`);
    }

    this[CORE] = new NativeURLSearchParams(init);
  }

  append(name, value) {
    requiredArguments('URLSearchParams.append', arguments.length, 2);
    this[CORE].append(`${name}`, `${value}`);
  }

  delete(name) {
    requiredArguments('URLSearchParams.delete', arguments.length, 1);
    this[CORE].delete(`${name}`);
  }

  get(name) {
    requiredArguments('URLSearchParams.get', arguments.length, 1);
    return this[CORE].get(`${name}`);
  }

  getAll(name) {
    requiredArguments('URLSearchParams.getAll', arguments.length, 1);
    return this[CORE].getAll(`${name}`);
  }

  has(name) {
    requiredArguments('URLSearchParams.has', arguments.length, 1);
    return this[CORE].has(`${name}`);
  }

  set(name, value) {
    requiredArguments('URLSearchParams.set', arguments.length, 2);
    this[CORE].set(`${name}`, `${value}`);
  }

  sort() {
    this[CORE].sort();
  }

  toString() {
    return this[CORE].toString();
  }

  get [Symbol.toStringTag]() {
    return 'URLSearchParams';
  }

  * keys() {
    const list = this[CORE].getIterableArray();
    for (const item of list) {
      yield item[0];
    }
  }

  * values() {
    const list = this[CORE].getIterableArray();
    for (const item of list) {
      yield item[1];
    }
  }

  * entries() {
    const list = this[CORE].getIterableArray();
    for (const item of list) {
      yield [ item[0], item[1] ];
    }
  }

  forEach(callback) {
    requiredArguments('URLSearchParams.forEach', arguments.length, 1);

    if (arguments.length >= 2 && typeof arguments[1] !== 'undefined') {
      callback = callback.bind(arguments[1]);
    }

    const list = this[CORE].getIterableArray();
    for (const [ key, value ] of list) {
      callback(value, key, this);
    }
  }
}

Object.defineProperties(URLSearchParams.prototype, {
  [Symbol.iterator]: {
    value: URLSearchParams.prototype.entries,
    enumerable: false,
    writable: true,
    configurable: true,
  },
});

class URL {
  [ON_PASSIVELY_UPDATE](structed) {
    this[STRUCTED] = structed;
  }

  constructor(url) {
    try {
      if (arguments.length >= 2) {
        this[INIT](url, arguments[1]);
      } else {
        this[INIT](url);
      }
    } catch (e) {
      throw new TypeError(e.message);
    }

    this[STRUCTED] = this[CORE].toStructed();
    this[CORE].setOnPassivelyUpdateFunction(
      this[ON_PASSIVELY_UPDATE].bind(this));
  }

  [INIT](url, base) {
    url = toUSVString(`${url}`);

    if (base !== undefined) {
      if (base instanceof URL) {
        this[CORE] = new NativeURL(url, base[CORE]);
      } else {
        this[CORE] = new NativeURL(url, toUSVString(`${base}`));
      }
    } else {
      this[CORE] = new NativeURL(url);
    }
  }

  get href() {
    return this[STRUCTED].href;
  }

  set href(value) {
    this[CORE].setHref(`${value}`);
  }

  get origin() {
    return this[STRUCTED].origin;
  }

  get protocol() {
    return this[STRUCTED].protocol;
  }

  set protocol(value) {
    this[CORE].setProtocol(`${value}`);
  }

  get username() {
    return this[STRUCTED].username;
  }

  set username(value) {
    this[CORE].setUsername(`${value}`);
  }

  get password() {
    return this[STRUCTED].password;
  }

  set password(value) {
    this[CORE].setPassword(`${value}`);
  }

  get host() {
    return this[STRUCTED].host;
  }

  set host(value) {
    this[CORE].setHost(`${value}`);
  }

  get hostname() {
    return this[STRUCTED].hostname;
  }

  set hostname(value) {
    this[CORE].setHostname(`${value}`);
  }

  get port() {
    return this[STRUCTED].port;
  }

  set port(value) {
    this[CORE].setPort(`${value}`);
  }

  get pathname() {
    return this[STRUCTED].pathname;
  }

  set pathname(value) {
    this[CORE].setPathname(`${value}`);
  }

  get search() {
    return this[STRUCTED].search;
  }

  set search(value) {
    this[CORE].setSearch(`${value}`);
  }

  get searchParams() {
    if (this[SEARCH_PARAMS]) return this[SEARCH_PARAMS];
    const params = this[CORE].getURLSearchParams();
    this[SEARCH_PARAMS] = new URLSearchParams(params);
    return this[SEARCH_PARAMS];
  }

  get hash() {
    return this[STRUCTED].hash;
  }

  set hash(value) {
    this[CORE].setHash(`${value}`);
  }

  toString() {
    return this[STRUCTED].href;
  }

  toJSON() {
    return this[STRUCTED].href;
  }

  get [Symbol.toStringTag]() {
    return 'URL';
  }

  [customInspectSymbol](depth, options) {
    const obj = {
      href: this.href,
      origin: this.origin,
      protocol: this.protocol,
      username: this.username,
      password: this.password,
      host: this.host,
      hostname: this.hostname,
      port: this.port,
      pathname: this.pathname,
      hash: this.hash,
      search: this.search,
      searchParams: this.searchParams,
    };

    return `URL ${inspect(obj, {
      ...options,
      depth: depth === null ? undefined : depth,
    })}`;
  }
}

const urlPrototypes = [
  'href',
  'origin',
  'protocol',
  'username',
  'password',
  'host',
  'hostname',
  'port',
  'pathname',
  'search',
  'searchParams',
  'hash',
  'toString',
  'toJSON',
];

for (const key of urlPrototypes) {
  const d = Object.getOwnPropertyDescriptor(URL.prototype, key);

  d.enumerable = true;
  if (d.value) {
    d.writable = true;
    d.configurable = true;
  }

  Object.defineProperty(URL.prototype, key, d);
}

const urlSearchParamsPrototypes = [
  'append',
  'delete',
  'get',
  'getAll',
  'has',
  'set',
  'sort',
  'toString',
];
for (const key of urlSearchParamsPrototypes) {
  const d = Object.getOwnPropertyDescriptor(URLSearchParams.prototype, key);

  d.enumerable = true;
  d.writable = true;
  d.configurable = true;

  Object.defineProperty(URLSearchParams.prototype, key, d);
}

module.exports = {
  URL,
  URLSearchParams,
};
