
# setenv GLOG_vmodule board_state=2,solver=1

BRIDGE_OBJS = interface_dds.o conditional_line.o line_finder.o card.o hand.o board_state.o solver.o hand_requirements.o deal.o

DLLBASE		= dds
STATIC_LIB	= lib$(DLLBASE).a

TESTS = basic_test hand_requirements_test deal_test line_finder_test conditional_line_test
EXES = ${TESTS}

#  -flto
COMMON_FLAGS = -Wall -fopenmp

CFLAGS = -O4 ${COMMON_FLAGS}
# CFLAGS = -O1 -g ${COMMON_FLAGS}
# PROF = -pg

# -flto
LDFLAGS = -fopenmp -lCatch2Main -lCatch2 -lglog

DDS_LDFLAGS = ${LDFLAGS} -Ldds-develop/src -l$(DLLBASE) -lboost_thread

%.o: %.cc *.h
	g++ ${CFLAGS} ${PROF} -c $<  -o $@ 

all run_basic_test: basic_test
	./basic_test --reporter compact --success -d yes

run_requirements_test: hand_requirements_test
	./hand_requirements_test --reporter compact --success -d yes

run_deal_test: deal_test
	./deal_test --reporter compact --success -d yes

run_dds_test: dds_test
	./dds_test --reporter compact --success -d yes Basic3

run_interface_dds_test: interface_dds_test
	./interface_dds_test --reporter compact --success -d yes

run_line_finder_test: line_finder_test
	./line_finder_test --reporter compact --success -d yes

run_board_state_test: board_state_test
	./board_state_test --reporter compact --success -d yes

run_conditional_line_test: conditional_line_test
	./conditional_line_test --reporter compact --success -d yes

all_tests: ${TESTS}

run_all_tests: run_requirements_test run_deal_test run_line_finder_test run_board_state_test run_basic_test run_conditional_line_test

# ./basic_test --durations yes

basic_test.o: basic_test.cc *.h
	g++ ${CFLAGS} ${PROF} -c $<  -o $@ 

basic_test: basic_test.o ${BRIDGE_OBJS}
	g++ -Wall ${PROF} -o $@ basic_test.o ${BRIDGE_OBJS} ${DDS_LDFLAGS}

line_finder_test: line_finder_test.o ${BRIDGE_OBJS}
	g++ -Wall ${PROF} -o $@ line_finder_test.o ${BRIDGE_OBJS} ${DDS_LDFLAGS}

board_state_test: board_state_test.o ${BRIDGE_OBJS}
	g++ -Wall ${PROF} -o $@ board_state_test.o ${BRIDGE_OBJS} ${DDS_LDFLAGS}

conditional_line_test: conditional_line_test.o ${BRIDGE_OBJS}
	g++ -Wall ${PROF} -o $@ conditional_line_test.o ${BRIDGE_OBJS} ${DDS_LDFLAGS}

test_play: test_play.cc ${BRIDGE_OBJS}
	g++ ${CFLAGS} ${PROF} $< ${BRIDGE_OBJS} -o $@ -lglog

hand_requirements_test.o: hand_requirements_test.cc *.h
	g++ ${CFLAGS} ${PROF} -c $<  -o $@ 

hand_requirements_test: hand_requirements_test.o ${BRIDGE_OBJS}
	g++ -Wall ${PROF} -o $@ hand_requirements_test.o ${BRIDGE_OBJS} ${DDS_LDFLAGS}

deal_test: deal_test.o ${BRIDGE_OBJS}
	g++ -Wall ${PROF} -o $@ deal_test.o ${BRIDGE_OBJS} ${DDS_LDFLAGS}

dds_test: dds_test.o ${BRIDGE_OBJS}
	g++ -Wall ${PROF} -o $@ dds_test.o ${BRIDGE_OBJS} ${DDS_LDFLAGS}

interface_dds_test: interface_dds_test.o ${BRIDGE_OBJS}
	g++ -Wall ${PROF} -o $@ interface_dds_test.o ${BRIDGE_OBJS} ${DDS_LDFLAGS}

clean:
	rm *.o ${EXES}

gprof:
	gprof ./basic_test gmon.out  > /tmp/out

backup: *.cc *.h
	mv backup backup.old
	mkdir backup
	cp Makefile *.{cc,h} *.pl backup
	mv backup.old backup/backup

tar:
	tar -cf bridge-solver.tar Makefile *.{cc,h,pl}
	gzip -f bridge-solver.tar

graph:
	cat /tmp/foo | ./html_from_line.pl /tmp/line2.html 
