
INC_DIR=./include
SRC_DIR=./src
OBJ_DIR=./obj
BIN_DIR=./bin
TST_DIR=./test

CXX=g++
CXXFLAGS=-I$(INC_DIR) -std=c++11
LIBS=-lpthread

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

topk: $(OBJ_DIR)/main.o
	$(CXX) -o $(BIN_DIR)/$@ $< $(LIBS)


$(OBJ_DIR)/%.o: $(TST_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%_test: $(OBJ_DIR)/%_test.o
	$(CXX) -o $(BIN_DIR)/$@ $< $(LIBS)

test: thread_pool_test topk_solver_test util_test
	ls $(BIN_DIR)/*_test | xargs -i sh -xc {}

clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(BIN_DIR)/*
