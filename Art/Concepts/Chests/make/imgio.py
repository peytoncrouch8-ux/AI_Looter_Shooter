import bpy, numpy as np
def load(path):
    im = bpy.data.images.load(path); im.colorspace_settings.name='Non-Color'
    w,h = im.size; a = np.empty(w*h*4, np.float32); im.pixels.foreach_get(a)
    bpy.data.images.remove(im)
    return np.flipud(a.reshape(h,w,4)).copy()
def save(arr, path):
    h,w = arr.shape[:2]
    im = bpy.data.images.new('o', w, h, alpha=True); im.colorspace_settings.name='Non-Color'
    im.alpha_mode='STRAIGHT'
    im.pixels.foreach_set(np.flipud(np.clip(arr,0,1)).astype(np.float32).ravel())
    im.filepath_raw=path; im.file_format='PNG'; im.save()
    bpy.data.images.remove(im)
def label(mask):
    from collections import deque
    h,w = mask.shape; lab = np.zeros((h,w), np.int32); n=0
    flat = mask.ravel(); L = lab.ravel()
    idx = np.flatnonzero(flat)
    for s in idx:
        if L[s]: continue
        n+=1; L[s]=n; q=deque([s])
        while q:
            p=q.popleft(); y,x=divmod(p,w)
            for qq,ok in ((p-1,x>0),(p+1,x<w-1),(p-w,y>0),(p+w,y<h-1)):
                if ok and flat[qq] and not L[qq]: L[qq]=n; q.append(qq)
    return lab, n
