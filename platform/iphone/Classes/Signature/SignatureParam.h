/*------------------------------------------------------------------------
 * (The MIT License)
 *
 * Copyright (c) 2008-2011 Rhomobile, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * http://rhomobile.com
 *------------------------------------------------------------------------*/

#ifndef __SignatureParam__
#define __SignatureParam__

/*
enum ComporessionFormat {
    CF_JPEG = 0,
    CF_PNG,
    CF_BMP
};
*/

#define CF_JPEG 0
#define CF_PNG 1
#define CF_BMP 2


struct SignatureParam {    
    unsigned int compressionFormat;
    const char* outputFormat;
    NSString* fileName;
    
    bool border;
    
    unsigned int penColor;
    float penWidth;
    unsigned int bgColor;
    int left;
    int top;
    unsigned int width;
    unsigned int height;
    
    bool setFullscreen;
};

#endif /* defined(__SignatureParam__) */