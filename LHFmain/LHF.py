import ctypes
from numpy.ctypeslib import ndpointer



#("dim", ctypes.POINTER(ctypes.c_int)),
class bettiBoundaryTableEntry(ctypes.Structure):
    _fields_ = [#("dim", ctypes.POINTER(ctypes.c_int)),
            ("dim", ctypes.c_int),
            ("birth", ctypes.c_double),
            ("death", ctypes.c_double)]

class bettiBoundaryTable(ctypes.Structure):
    _fields_ = [ ("size", ctypes.c_int),
                 ("bettis", ctypes.c_void_p)]

class pipePacketAtt(ctypes.Structure):
    _fields_ = [ ("size", ctypes.c_int),
                 ("LHF_size", ctypes.c_int),
                 ("LHF_dim", ctypes.c_int),
                 ("workData_size", ctypes.c_int),
                 ("bettiTable", ctypes.c_void_p),
                 ("inputData", ctypes.c_void_p),
                 ("distMatrix", ctypes.c_void_p),
                 ("workData", ctypes.c_void_p),
                 ("centroidLabels", ctypes.c_void_p),
                 ("stats", ctypes.c_char_p),
                 ("runLog", ctypes.c_char_p)]

class LHF:
	#Use RTLD_LAZY mode due to undefined symbols
    lib = ctypes.CDLL("./libLHFlib.so",mode=1)
    args = {"reductionPercentage":"10","maxSize":"2000","threads":"30","threshold":"250","scalar":"2.0","mpi": "0","mode": "standard","dimensions":"1","iterations":"250","pipeline":"","inputFile":"None","outputFile":"output","epsilon":"5","lambda":".25","debug":"0","complexType":"simplexArrayList","clusters":"20","preprocessor":"","upscale":"false","seed":"-1","twist":"false","collapse":"false"}    
    data = []

    ## Some notes here:
    ##
    ##  These are returned by C++ in a C function; need to be wrapped into a C structure in LHF 
    ##      This indicates we can return only necessary (C-format) entries - TBD
    ##
    ##  Some entries have known sizes (based on LHF class):
    ##          bettiBoundaryTable = (? x 4) - betti boundary struct
    ##          workData = ((? <= LHF.size) x LHF.dim) - centroids
    ##          centroidLabels = (LHF.size x 1) - centroid labels
    ##          inputData = (LHF.size x LHF.dim) - original data
    ##          distMatrix = (LHF.size x LHF.size) - distance matrix
    ##          
    ##
    
    def __init__(self, data):  
        self.args["datadim"] = len(data[0])
        self.args["datasize"] = len(data)
        
        self.data = data
        
        self.lib.testFunc.argtypes = [ctypes.c_int, ctypes.c_char_p]
        self.lib.testFunc.restype = None

        self.lib.pyRunWrapper.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.POINTER(ctypes.c_double)]
        self.lib.pyRunWrapper.restype = None

        self.lib.pyRunWrapper2.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.POINTER(ctypes.c_double)]
        self.lib.pyRunWrapper2.restype = ctypes.c_void_p
   
    def allocation(size):
        class pybettiBoundaryTableEntry(ctypes.Structure):
            _fields_ = [("dim", ctypes.c_int),
                    #("Bettidim", ctypes.POINTER(ctypes.c_uint)),
                    ("Bettidim", ctypes.c_double * size),
                    ("birth", ctypes.c_double * size),
                    ("death", types.c_double * size)]


    def args2string(self, inList):
        ret = ""
        
        for a in inList:
            ret += a+" "
            ret += str(inList[a])+" "
            
        return ret.encode('utf-8')
        
    def runPH(self):
        #Create char* for passing to C++
        temp = self.args2string(self.args)
        
        return self.lib.pyRunWrapper(len(temp),ctypes.c_char_p(temp), self.data.ctypes.data_as(ctypes.POINTER(ctypes.c_double)))


    def testFunc(self, num, st):
        return self.lib.testFunc(num, ctypes.c_char_p(st.encode('utf-8')))

    def runPH2(self):
        #Create char* for passing to C++
        temp = self.args2string(self.args)
        
        
        #retPH = pipePacketAtt.from_address(self.lib.pyRunWrapper2(len(temp),ctypes.c_char_p(temp), self.data.ctypes.data_as(ctypes.POINTER(ctypes.c_double))))
        retPH = pipePacketAtt.from_address(self.lib.pyRunWrapper2(len(temp),ctypes.c_char_p(temp), self.data.ctypes.data_as(ctypes.POINTER(ctypes.c_double))))
        

        print("Total Boundaries",retPH.size)
        #print(retPH.ident)

        #Reconstruct the boundary table array from the address?
        
        bettiBoundaryTableEntries = type("array", (ctypes.Structure, ), {
            # data members
            "_fields_" : [("arr", bettiBoundaryTableEntry * retPH.size)]
        })


        retBounds = bettiBoundaryTableEntries.from_address(retPH.bettiTable)

        # for i in range(retPH.size):
        #     print(retBounds.arr[i].dim,retBounds.arr[i].birth,retBounds.arr[i].death)

        # print("inputData size",retPH.LHF_size)
        # print("inputData dim",retPH.LHF_dim)
        
        inputDataEntries = type("array", (ctypes.Structure, ), {
            # data members
            "_fields_" : [("arr", ctypes.c_double * (retPH.LHF_size * retPH.LHF_dim))]
        }) 

        retinputData = inputDataEntries.from_address(retPH.inputData)

        # for i in range(retPH.LHF_size * retPH.LHF_dim):
        #     print(i, ": ", retinputData.arr[i])

        distMatrixEntries = type("array", (ctypes.Structure, ), {
            # data members
            "_fields_" : [("arr", ctypes.c_double * (retPH.LHF_size * retPH.LHF_size))]
        }) 

        retdistMatrix = distMatrixEntries.from_address(retPH.distMatrix)

        # for i in range(retPH.LHF_size * retPH.LHF_size):
        #     if(i < 100):
        #         print(i, ": ", retdistMatrix.arr[i])
            # if(i % 100 == 0):
            #     print(i, ": ", retdistMatrix.arr[i])
            # print(i, ": ", retdistMatrix.arr[i])

        centroidLabelsEntries = type("array", (ctypes.Structure, ), {
            # data members
            "_fields_" : [("arr", ctypes.c_double * (retPH.LHF_size))]
        }) 

        retcentroidLabels = centroidLabelsEntries.from_address(retPH.centroidLabels)

        # print("workData size",retPH.workData_size)
        # print("LHF dim",retPH.LHF_dim)

        workDataEntries = type("array", (ctypes.Structure, ), {
            # data members
            "_fields_" : [("arr", ctypes.c_double * (retPH.workData_size * retPH.LHF_dim))]
        }) 

        retworkData= workDataEntries.from_address(retPH.workData)

        # for i in range(retPH.workData_size * retPH.LHF_dim):
        #     print(i, ": ", retworkData.arr[i])


        pystats = str(retPH.stats).replace('\\n', '\n').replace('b\'', '').replace('\'','')
        # print(pystats)

        pyrunLog = str(retPH.runLog).replace('\\n', '\n').replace('b\'', '').replace('\'','')
        # print(pyrunLog)

        return
