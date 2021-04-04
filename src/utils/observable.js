import Vue from 'vue'
import {isEqual} from './is-euqal'

export function observable (target, name, descriptor) {
  const v = descriptor.initializer && descriptor.initializer.call(this);

  function init (instance) {
    const id = `$$observable-${name}`
    if (!instance[id]) {
      Object.defineProperty(instance, id, {
        value: Vue.observable({v}),
        enumerable: false,
        writeable: false,
        configurable: false
      });
    }

    return instance[id]
  }
  
  return {
    configurable: true,
    enumerable: true,
    get () {
      const obj = init(this)
      return obj.v 
    },
    set (v) {
      const obj = init(this)
      if (!isEqual(v, obj.v)) {
        obj.v = v
      }
    }
  }
}

export function observable_saveable (target, name, descriptor) {
  let initVal 

  if (descriptor.initializer) {
    initVal = descriptor.initializer()
  }

  function init (instance) {
    let obj = instance[`$observable-${name}`]
    if (!obj) {
      obj = Vue.observable({v: initVal})
      instance[`$observable-${name}`] = obj
    }
    return obj
  }
  
  return {
    configurable: true,
    enumerable: true,
    get () {
      const obj = init(this)
      return obj.v 
    },
    set (v) {
      const obj = init(this)
      if (!isEqual(v, obj.v)) {
        obj.v = v
      }
    }
  }
}
