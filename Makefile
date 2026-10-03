.PHONY: all all_tests run_all_tests clean run_basic_test run_requirements_test run_deal_test run_dds_test run_interface_dds_test run_line_finder_test run_board_state_test run_conditional_line_test

all:
	cmake -B build
	cmake --build build -j4

all_tests: all

run_all_tests: all
	ctest --test-dir build --output-on-failure

run_basic_test: all
	./build/basic_test --reporter compact --success -d yes

run_requirements_test: all
	./build/hand_requirements_test --reporter compact --success -d yes

run_deal_test: all
	./build/deal_test --reporter compact --success -d yes

run_dds_test: all
	./build/dds_test --reporter compact --success -d yes Basic3

run_interface_dds_test: all
	./build/interface_dds_test --reporter compact --success -d yes

run_line_finder_test: all
	./build/line_finder_test --reporter compact --success -d yes

run_board_state_test: all
	./build/board_state_test --reporter compact --success -d yes

run_conditional_line_test: all
	./build/conditional_line_test --reporter compact --success -d yes

clean:
	rm -rf build

%:
	cmake -B build
	cmake --build build --target $@ -j4
