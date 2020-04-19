
INC_DIR=./include
SRC_DIR=./src
OBJ_DIR=./obj
BIN_DIR=./bin
TST_DIR=./test

.DEFAULT_GOAL := topk

mkdirs:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

CXX=g++
CXXFLAGS=-I$(INC_DIR) -std=c++11
LIBS=-lpthread

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp mkdirs
	$(CXX) $(CXXFLAGS) -c -o $@ $<

topk: $(OBJ_DIR)/main.o mkdirs
	$(CXX) -o $(BIN_DIR)/$@ $< $(LIBS)


$(OBJ_DIR)/%.o: $(TST_DIR)/%.cpp mkdirs 
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%_test: $(OBJ_DIR)/%_test.o mkdirs
	$(CXX) -o $(BIN_DIR)/$@ $< $(LIBS)

test: thread_pool_test topk_solver_test util_test
	echo $^ | tr ' ' '\n' | xargs -i sh -xc $(BIN_DIR)/{}

ptest: performance_test
	$(BIN_DIR)/performance_test

clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(BIN_DIR)/*

