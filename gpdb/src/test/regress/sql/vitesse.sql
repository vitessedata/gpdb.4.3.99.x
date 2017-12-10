-- ------------------------------------------------
-- Test hash left join where the results should be
-- different depending on whether a condition 
-- is a filter or a join condition.
--

create temp table t1 as (select i
    , i::bigint bi
    , i / 10000 as j
    , case i%5 when 0 then null else i % 5 end as k
    , case i%3 when 0 then null when 1 then -i else i end as m
    , i / 10 as n
    , case i%11 when 0 then null else ('tt.'||i)::varchar end as t
    from (select * from generate_series(1, 1000000) i) foo)
    distributed randomly;

analyze t1;

-- b.m is a filter case. we get less rows.
select 'expect 6', count(*) from (
SELECT a.i AS ai, b.i AS bi, b.m
   FROM t1 a
   LEFT JOIN t1 b ON a.i = b.i 
WHERE (b.m > 0 OR b.m IS NULL)
  AND a.i < 10
) foo;


-- b.m is a condition in a left join. we get more rows.
select 'expect 9', count(*) from (
SELECT a.i AS ai, b.i AS bi, b.m
   FROM t1 a
   LEFT JOIN t1 b ON a.i = b.i AND (b.m > 0 OR b.m IS NULL)
  WHERE a.i < 10
) foo;


