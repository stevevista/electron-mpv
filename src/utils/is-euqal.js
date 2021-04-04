export function isEqual (objA, objB) {
  if (objA === objB) return objA !== 0 || 1 / objA === 1 / objB;
  if (objA == null || objB == null) return objA === objB;

  if (Object.prototype.toString.call(objA) !== Object.prototype.toString.call(objB)) return false;

  switch (Object.prototype.toString.call(objA)) {
    case '[object RegExp]':
    case '[object String]':
      return '' + objA === '' + objB;
    case '[object Number]':
      return +objA === 0 ? 1 / +objA === 1 / objB : +objA === +objB;
    case '[object Date]':
    case '[object Boolean]':
      return +objA === +objB;
    case '[object Array]':
      for (let i = 0; i < Math.max(objA.length, objB.length); i++) {
        if (!isEqual(objA[i], objB[i])) return false;
      }
      return true;
    case '[object Object]':
      let keys = Object.keys(objA);
      for (let i = 0; i < keys.length; i++) {
        if (!isEqual(objA[keys[i]], objB[keys[i]])) return false;
      }

      keys = Object.keys(objB);
      for (let i = 0; i < keys.length; i++) {
        if (!isEqual(objA[keys[i]], objB[keys[i]])) return false;
      }

      return true;
    default:
      return false;
  }
}
