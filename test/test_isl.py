from __future__ import division

import islpy as isl


def test_basics():
    dt = isl.dim_type

    ctx = isl.Context()
    space = isl.Space.create_from_names(ctx, set=["a", "b"])

    bset = (isl.BasicSet.universe(space)
            .add_constraint(isl.Constraint.eq_from_names(
                space, {"a": -1, "b": 2}))
            .add_constraint(isl.Constraint.ineq_from_names(
                space, {"a": 1, 1: -10}))
            .add_constraint(isl.Constraint.ineq_from_names(
                space, {"a": -1, 1: 42}))
            .project_out(dt.set, 1, 1))

    bset2 = isl.BasicSet(  # noqa
            "{[i] : exists (a : i = 2a and i >= 10 and i <= 42)}",
            context=ctx)

    points = []
    bset.foreach_point(points.append)

    for pt in points:
        print(pt)


def test_error_on_invalid_index():
    ctx = isl.Context()
    my_set = isl.Set("{ [k, l] : 3l >= -k and 3l <= 10 - k "
                   "and k >=0 and k <= 2 }", context=ctx)
    p = my_set.sample_point()
    try:
        p.get_coordinate_val(isl.dim_type.set, 99999999)
    except RuntimeError:
        pass
    else:
        assert False


def test_pwqpoly():
    def term_handler(term):
        print(term.get_coefficient_val())

    def piece_handler(set, qpoly):
        qpoly.foreach_term(term_handler)

    pwqp = isl.PwQPolynomial('[n] -> { n }')
    pwqp.foreach_piece(piece_handler)


def test_id_user():
    ctx = isl.Context()
    foo = isl.Id("foo", context=ctx)  # noqa
    t = (1, 2)
    bar = isl.Id("bar", t, context=ctx)

    assert bar.user is t


def test_val():
    for src in [17, "17"]:
        v = isl.Val(src)
        print v
        assert v == 17
        assert v.to_python() == 17


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        exec(sys.argv[1])
    else:
        from py.test.cmdline import main
        main([__file__])
