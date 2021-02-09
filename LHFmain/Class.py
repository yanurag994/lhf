import ctypes

dimension = 3

class point2d(ctypes.Structure):
    _fields_ = ("x0", ctypes.c_double), ("x1", ctypes.c_double)
    

class setCppU(ctypes.Structure):
    _fields_ = [("setU", ctypes.c_uint)]

class setCppD(ctypes.Structure):
    _fields_ = [("setD", ctypes.c_double)]

class vectordouble(ctypes.Structure):
    _fields_ = [("vectorD", ctypes.c_double)]

class vectorUnsigned(ctypes.Structure):
    _fields_ = [("vectorU", ctypes.c_uint)]

class weights(ctypes.Structure):
    _fields_ = [("setD", ctypes.c_double), ("greater", ctypes.c_double)]

class array2d(ctypes.Structure):
    _fields_ = [("a", ctypes.c_int),
                ("b", ctypes.c_float),
                ("arr", point2d * 10)] ##dynamic



########################################################

# class simplexBase(ctypes.Structure): ###needed? 
#     _fields_ = 

class bettiBoundaryTableEntry(ctypes.Structure):
    _fields_ = [("Bettidim", ctypes.c_uint),
                ("birth", ctypes.c_double),
                ("death", ctypes.c_double),
                ("boundaryPoints", setCppU * 10)] ##dynamic


class pipePacket(ctypes.Structure):
    _fields_ = [("bettiBoundaryTableEntry", bettiBoundaryTableEntry * 10), ##dynamic
                ("ident", ctypes.c_char_p),
                ("stats", ctypes.c_char_p),
                ("runLog", ctypes.c_char_p),
                ("workData", vectordouble * 10), ###dynamic
                ("centroidLabels", vectorUnsigned * 10), ##dynamic
                ("inputData", vectordouble * 10), ##dynamic
                ("distMatrix", vectordouble * 10), ##dynamic
                #("simplexBase", simplexBase * 10),
                ("boundaries", vectorUnsigned * 10), ##dynamic
                ("weights", weights * 10), ##dynamic
                ("bettiOutput", ctypes.c_char_p)] 
                #getSize?
                #getStats?
                
                
class argMap(ctypes.Structure):
    _fields_ = [("key", ctypes.c_char_p),
                ("value", ctypes.c_char_p)]
                
class argArray(ctypes.Structure):
    _fields_ = [("arr", argMap * 22)]

class LHF:
    lib = ctypes.cdll.LoadLibrary("./libLHFlib.so")
    dim = 0
    size = 0
    point = 0
    array = 0
    
    def __init__(self):  
        self.dim = 0
        self.size = 0
         
    def configData(self, data):
        self.dim = len(data[0])
        self.size = len(data)
        
        print("Created data size of d:",self.dim,", n:",self.size)

        self.point = type("point", (ctypes.Structure, ), {
            
            # data members
            "_fields_" : [("x", ctypes.c_double * self.dim)]
        })
        
        self.array = type("array", (ctypes.Structure, ), {
            
            # data members
            "_fields_" : [("arr", self.point * self.size)]
        })
        
        self.lib.testFunc.argtypes = [ctypes.c_int]
        self.lib.testFunc.restype = None

        self.lib.pyRunWrapper.argtypes = [argArray, ctypes.Structure]
        self.lib.pyRunWrapper.restype = None
        
    def getDefaultArgs(self):
        return {"reductionPercentage":"10","maxSize":"2000","threads":"30","threshold":"250","scalar":"2.0","mpi": "0","mode": "standard","dimensions":"1","iterations":"250","pipeline":"","inputFile":"None","outputFile":"output","epsilon":"5","lambda":".25","debug":"0","complexType":"simplexArrayList","clusters":"20","preprocessor":"","upscale":"false","seed":"-1","twist":"false","collapse":"false"}    
        
    def list2map(self, inList):
        i = 0
        retArray = argArray()
        for a in inList:            
            retArray.arr[i] = argMap(ctypes.c_char_p(a.encode('utf-8')), ctypes.c_char_p(inList[a].encode('utf-8')))
            i+=1
            
        return retArray
        
    def data2array(self, inData):
        i = 0
        retArray = self.array()
        for row in inData:
            curPoint = self.point()
            
            #populate dimensions
            d = 0
            for dim in row:
                curPoint.x[d] = ctypes.c_double(dim)
                d+=1
                
            retArray.arr[i] = curPoint;
            i+=1;
        
        return retArray;
        
        
        
    def pyRunWrapper(self, args, data):
        print("Running LHF")      
        
        return self.lib.pyRunWrapper(self.list2map(args), self.data2array(data))

    def testFunc(self, num):
        return self.lib.testFunc(num)


