module github.com/openucx/ucx/bindings/go/examples/perftest

require (
	github.com/openucx/ucx/bindings/go/src/cuda v0.0.0-00010101000000-000000000000
	github.com/openucx/ucx/bindings/go/src/ucx v0.0.0-00010101000000-000000000000
)

replace github.com/openucx/ucx/bindings/go/src/ucx => ../../ucx
replace github.com/openucx/ucx/bindings/go/src/cuda => ../../cuda