all:
	@make all -C lib

test: all
	@bash run-test.sh

testv: all
	@bash run-test.sh -v

clean:
	@make clean -C lib

cleanup: clean
	@rm -rf work/*
	@rm -rf results/*
