version becomes `100`

add line: `precision mediump float;` underneath version

`in` becomes `varying`

no `out` in fragment shader. set final color to `gl_FragColor` variable

float literals *must* have decimals

`texture()` becomes `texture2D()`

no `texelFetch`, need to use `texture2D` with normalized coords
