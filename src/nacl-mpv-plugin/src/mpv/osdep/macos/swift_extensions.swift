/*
 * This file is part of mpv.
 *
 * mpv is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * mpv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with mpv.  If not, see <http://www.gnu.org/licenses/>.
 */

import Cocoa

extension NSDeviceDescriptionKey {
    static let screenNumber = NSDeviceDescriptionKey("NSScreenNumber")
}

extension NSScreen {

    public var displayID: CGDirectDisplayID {
        get {
            return deviceDescription[.screenNumber] as? CGDirectDisplayID ?? 0
        }
    }

    public var displayName: String? {
        get {
            var name: String? = nil
            var object: io_object_t
            var iter = io_iterator_t()
            let matching = IOServiceMatching("IODisplayConnect")
            let result = IOServiceGetMatchingServices(kIOMasterPortDefault, matching, &iter)

            if result != KERN_SUCCESS || iter == 0 { return nil }

            repeat {
                object = IOIteratorNext(iter)
                if let info = IODisplayCreateInfoDictionary(object, IOOptionBits(kIODisplayOnlyPreferredName)).takeRetainedValue() as? [String:AnyObject],
                    (info[kDisplayVendorID] as? UInt32 == CGDisplayVendorNumber(displayID) &&
                    info[kDisplayProductID] as? UInt32 == CGDisplayModelNumber(displayID) &&
                    info[kDisplaySerialNumber] as? UInt32 ?? 0 == CGDisplaySerialNumber(displayID))
                {
                    if let productNames = info["DisplayProductName"] as? [String:String],
                       let productName = productNames.first?.value
                    {
                        name = productName
                        break
                    }
                }
            } while object != 0

            IOObjectRelease(iter)
            return name
        }
    }
}

extension NSColor {

    convenience init(hex: String) {
        let int = Int(hex.dropFirst(), radix: 16) ?? 0
        let alpha = CGFloat((int >> 24) & 0x000000FF)/255
        let red   = CGFloat((int >> 16) & 0x000000FF)/255
        let green = CGFloat((int >> 8)  & 0x000000FF)/255
        let blue  = CGFloat((int)       & 0x000000FF)/255

        self.init(calibratedRed: red, green: green, blue: blue, alpha: alpha)
    }
}

extension Bool {

    init(_ int32: Int32) {
        self.init(int32 != 0)
    }
}

extension Int32 {

    init(_ bool: Bool) {
        self.init(bool ? 1 : 0)
    }
}
