#!/usr/bin/env python3

import numpy as np
from PIL import Image
import os

# CREATES PALETTE FROM IMAGE AND SAVES IT TO A TEXTURE

def img2palette(arr: np.ndarray) -> np.ndarray:
    if arr.shape[2]==4:
        arr = sanitize(arr.reshape(-1,4))
    else:
        arr = sanitize(arr.reshape(-1,3))

    return np.unique(arr,axis=0)

def makePalette(img: np.ndarray, palette_fname: str = "") -> np.ndarray:
    # iterates through list of files and compiles palette 
    # uses all PNG files in path
    # if palette_fname is given, will save palette to txt file with that name
    
    palette = None
    imgPalette = img2palette(img)
    if palette is not None:
        palette = np.unique(np.concatenate([palette, imgPalette]),axis=0)
    else:
        palette = imgPalette

    if palette_fname:
        np.savetxt(palette_fname,palette,fmt='%i',delimiter=',')
    return palette

def sanitize(palette: np.ndarray) -> np.ndarray:
    # takes palette, removes any color with alpha=0 and removes 4th channel
    return palette[palette[:,-1]!=0][:,:3]

def findCol(col:np.ndarray, palette:np.ndarray) -> np.ndarray: # col should be np.array in form [R,G,B], palette should be array of cols
    # finds the color in "palette" that's closest to "col"

    # RESEARCH use OKLab
    distances = np.sum(((palette-col)*np.array([.299, .587, .114]))**2, axis=1)
    return palette[np.argmin(distances)]

def getPNG(path: str) -> np.ndarray:
    # returns numpy array of image
    result = np.array(Image.open(path).convert("RGBA"))
    return result

def readPalette(filepath: str) -> np.ndarray:
    # reads palette from disk
    if os.path.exists(filepath):
        palette = np.loadtxt(filepath,dtype=int,delimiter=",")
        if palette.shape[-1]==4:
            palette = palette[:,:3] # reshape if there's an alpha val
        return palette
    else:
        raise ValueError("palette file '{}' does not exist".format(filepath))

def paletteToTexture(palette, outpath):
    N_COLORS_R = 4
    N_COLORS_G = 4
    N_COLORS_B = 4

    img = Image.new("RGB", (N_COLORS_R * N_COLORS_G, N_COLORS_B), "white")
    testImg = Image.new("RGB", (N_COLORS_R * N_COLORS_G, N_COLORS_B), "white")

    rvals = np.linspace(0, N_COLORS_R, N_COLORS_R)
    gvals = np.linspace(0, N_COLORS_G, N_COLORS_G)
    bvals = np.linspace(0, N_COLORS_B, N_COLORS_B)
    for r in range(N_COLORS_R):
        for g in range(N_COLORS_G):
            for b in range(N_COLORS_B):
                # rgb = np.array([r/N_COLORS_R,g/N_COLORS_G, b/N_COLORS_B]) * 255
                rgb = np.array([rvals[r]/N_COLORS_R,gvals[g]/N_COLORS_G, bvals[b]/N_COLORS_B]) * 255
                p = findCol(rgb, palette)
                x = r + (g * N_COLORS_G)
                y = b 
                # print("closest color to ", list(rgb), " is ", list(p))
                img.putpixel((x,y), tuple(p))
                testImg.putpixel((x,y), tuple(rgb.astype(int))) 
    img.save(outpath)
    testImg.save("test.png")
    print("saved to " + outpath)


def main(path:str, outpath):
    if outpath is None:
        outpath = ""

    palette = makePalette(getPNG(path), outpath)
    paletteToTexture(palette, outpath)

if __name__ == "__main__":
    import sys

    if (len(sys.argv) != 3 and len(sys.argv) != 2):
        print("expected 1 or 2 args: file path and out path (optional)")
    else:
        outpath = "data/texture/palette.png"
        if (len(sys.argv)==3):
            outpath = sys.argv[2]
        inpath = sys.argv[1]

        main(inpath, outpath)


