\c template1
drop database dgtest;
create database dgtest;

\c dgtest

create schema s1;
create schema s2;

create table t1 (i int, t text);
insert into t1 select i, 't' || i from generate_series(1, 100) i;

create table part1 (a text, b int, c char(10))
distributed by (a) 
partition by range (b)
(
    partition aa start (1) end (10),
    partition bb start (10) end (100),
    default partition big
);
insert into part1 select t, i, t from t1;

create table s1.part(i int, j int, k int, t text)
distributed by (j)
partition by range (i)
subpartition by range (k) subpartition template ( start (1) end (300) every (100))
(
    partition aa start (1) end (10),
    partition bb start (10) end (100),
    default partition big
);

insert into s1.part select i, i+10, i*2, 'xxx' || i from generate_series(1, 100) i;


