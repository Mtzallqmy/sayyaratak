#!/usr/bin/env python3
# glb2car.py - convert the CC0 tevy.dev supercar GLB into Sayyaratak's packed car format.
# Output binary (little-endian):
#   magic 'SYCR' u32, version u32, numParts u32
#   per part: nameLen u32 + name bytes, flags u32 (1=glass 2=lights 4=paint 8=wing),
#             pivot[3] f32, vcount u32, icount u32,
#             vertices vcount*10 f32 (pos3, nrm3, col4), indices icount u32
import json, struct, sys, math

CT_SIZE = {5120:1, 5121:1, 5122:2, 5123:2, 5125:4, 5126:4}
CT_FMT  = {5120:'b', 5121:'B', 5122:'h', 5123:'H', 5125:'I', 5126:'f'}
NCOMP   = {'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}

# materials that only carry KTX2 textures we cannot decode: substitute colors
TEX_FALLBACK = {
    'mesh': (0.04,0.04,0.045), 'carbon': (0.030,0.030,0.033),
    'leather_quilted': (0.10,0.062,0.040), 'brake_disc': (0.45,0.45,0.48),
}

def mat_identity(): return [[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]
def mat_mul(a,b):
    return [[sum(a[i][k]*b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]
def mat_trs(n):
    t = n.get('translation',[0,0,0]); q = n.get('rotation',[0,0,0,1]); s = n.get('scale',[1,1,1])
    x,y,z,w = q
    R = [[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
         [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
         [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]]
    m = mat_identity()
    for i in range(3):
        for jj in range(3): m[i][jj] = R[i][jj]*s[jj]
        m[i][3] = t[i]
    return m
def mat_xform(m, p):
    return tuple(m[i][0]*p[0]+m[i][1]*p[1]+m[i][2]*p[2]+m[i][3] for i in range(3))
def mat_xform_n(m, p):
    return tuple(m[i][0]*p[0]+m[i][1]*p[1]+m[i][2]*p[2] for i in range(3))

class Glb:
    def __init__(self, path):
        d = open(path,'rb').read()
        clen, _ = struct.unpack('<II', d[12:20])
        self.j = json.loads(d[20:20+clen])
        off = 20+clen
        self.bin = b''
        if off < len(d):
            blen, _ = struct.unpack('<II', d[off:off+8])
            self.bin = d[off+8:off+8+blen]
    def accessor(self, idx):
        a = self.j['accessors'][idx]
        bv = self.j['bufferViews'][a['bufferView']]
        base = bv.get('byteOffset',0) + a.get('byteOffset',0)
        n = a['count']; ct = a['componentType']; ty = a['type']
        nc = NCOMP[ty]; sz = CT_SIZE[ct]*nc
        stride = bv.get('byteStride') or sz
        norm = a.get('normalized', False)
        out = []
        for i in range(n):
            o = base + i*stride
            vals = struct.unpack_from('<'+CT_FMT[ct]*nc, self.bin, o)
            if norm:
                if ct == 5120: vals = tuple(max(v/127.0,-1.0) for v in vals)
                elif ct == 5121: vals = tuple(v/255.0 for v in vals)
                elif ct == 5122: vals = tuple(max(v/32767.0,-1.0) for v in vals)
                elif ct == 5123: vals = tuple(v/65535.0 for v in vals)
            out.append(vals)
        return out

def mat_color(j, midx, mats_emissive):
    m = j['materials'][midx]
    name = m.get('name','')
    pbr = m.get('pbrMetallicRoughness',{})
    if 'baseColorFactor' in pbr: c = list(pbr['baseColorFactor'][:3])
    elif name in TEX_FALLBACK: c = list(TEX_FALLBACK[name])
    elif 'baseColorTexture' in pbr: c = [0.5,0.5,0.5]
    else: c = [1.0,1.0,1.0]
    if 'emissiveFactor' in m and max(m['emissiveFactor']) > 0.01:
        mats_emissive.add(name)
    return name, c

def convert(in_path, out_path, parked=False):
    g = Glb(in_path); j = g.j
    nodes = j['nodes']
    mats_emissive = set()
    # classify material names
    LIGHT_MATS = {'light_front','light_rear','light_drl','light_amber','reflector_red','lens_red','lens_clear'}
    GLASS_MATS = {'glass','glass_clear'}
    # parts: name -> (flags, pivotNodeName or None)
    parts = {}
    def part_for(nodename, matname):
        # returns (partname, flags, pivotnode)
        if matname == 'paint':
            if nodename == 'wing_skin': return ('wing', 4|8, None)
            return ('body', 4, None)
        if nodename == 'wing_skin': return ('wing_trim', 8, None)
        if matname in LIGHT_MATS or matname in mats_emissive: return ('lights', 2, None)
        if matname in GLASS_MATS or matname.startswith('glass'): return ('glass', 1, None)
        if nodename == 'wheel_FL_mesh': return ('wheel_FL', 0, 'steer_FL')
        if nodename == 'wheel_FR_mesh': return ('wheel_FR', 0, 'steer_FR')
        if nodename == 'wheel_RL_mesh': return ('wheel_RL', 0, 'wheel_RL')
        if nodename == 'wheel_RR_mesh': return ('wheel_RR', 0, 'wheel_RR')
        if nodename == 'caliper_FL': return ('steer_L', 0, 'steer_FL')
        if nodename == 'caliper_FR': return ('steer_R', 0, 'steer_FR')
        return ('trim', 0, None)

    # world matrices
    world = {}
    def walk(i, parent):
        m = mat_mul(parent, mat_trs(nodes[i]))
        world[i] = m
        for c in nodes[i].get('children',[]): walk(c, m)
    for r in j['scenes'][0]['nodes']: walk(r, mat_identity())

    # pivots: world position of wheel/steer nodes
    pivot_pos = {}
    for i,n in enumerate(nodes):
        nm = n.get('name','')
        if nm in ('steer_FL','steer_FR','wheel_RL','wheel_RR'):
            pivot_pos[nm] = mat_xform(world[i], (0,0,0))

    for i,n in enumerate(nodes):
        if 'mesh' not in n: continue
        nm = n.get('name','')
        mesh = j['meshes'][n['mesh']]
        m = world[i]
        for prim in mesh['primitives']:
            if prim.get('mode',4) != 4: continue
            matname, col = mat_color(j, prim['material'], mats_emissive)
            pname, flags, pivotnode = part_for(nm, matname)
            if pname not in parts:
                parts[pname] = {'flags':flags,'pivot':pivotnode,'v':[],'idx':[]}
            P = parts[pname]
            pos = g.accessor(prim['attributes']['POSITION'])
            nrm = g.accessor(prim['attributes']['NORMAL']) if 'NORMAL' in prim['attributes'] else [(0,1,0)]*len(pos)
            idx = [v[0] for v in g.accessor(prim['indices'])] if 'indices' in prim else list(range(len(pos)))
            vbase = len(P['v'])
            for k in range(len(pos)):
                p = mat_xform(m, pos[k])
                nn = mat_xform_n(m, nrm[k])
                l = math.sqrt(nn[0]**2+nn[1]**2+nn[2]**2) or 1.0
                P['v'].append((p[0],p[1],p[2], nn[0]/l,nn[1]/l,nn[2]/l, col[0],col[1],col[2],1.0))
            P['idx'].extend(vbase+v for v in idx)

    order = ['body','trim','wing','wing_trim','lights','glass','wheel_FL','wheel_FR','wheel_RL','wheel_RR','steer_L','steer_R']
    out = struct.pack('<III', 0x52435953, 1, len(parts))  # 'SYCR' LE
    n_total_v = 0
    for pname in order:
        if pname not in parts: continue
        P = parts[pname]
        pv = pivot_pos.get(P['pivot'], (0,0,0)) if P['pivot'] else (0,0,0)
        n_total_v += len(P['v'])
        nb = pname.encode()
        out += struct.pack('<I', len(nb)) + nb
        out += struct.pack('<I3fII', P['flags'], pv[0],pv[1],pv[2], len(P['v']), len(P['idx']))
        out += struct.pack('<%df' % (len(P['v'])*10), *[x for v in P['v'] for x in v])
        out += struct.pack('<%dI' % len(P['idx']), *P['idx'])
    open(out_path,'wb').write(out)
    print(f"{out_path}: parts={[p for p in order if p in parts]} verts={n_total_v} bytes={len(out)}")
    print("pivots:", {k: ['%.3f'%x for x in v] for k,v in pivot_pos.items()})

if __name__ == '__main__':
    convert(sys.argv[1], sys.argv[2])
