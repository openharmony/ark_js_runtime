try {
    var array = new Uint8Array(1000000);
    var res = String.fromCharCode.apply(null, array);
} catch (e) {
    if ((e instanceof RangeError)) {
        print("stack overflow!");
    }
}