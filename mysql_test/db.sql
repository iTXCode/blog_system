drop database if exists blog_system;
create database blog_system;
use blog_system;

drop table if exists  blog_table; 
create table blog_table
(
    blog_id int not null primary key auto_increment comment '博客编号',
    -- 分布式系统下生成唯一主键
    -- 1.使用时间戳
    -- 2.使用机房id
    -- 3.使用主机ip
    -- 4.使用随机数
    -- 为了效率,牺牲了数据的强一致性
    -- 效率、一致性、容错三者在系统中做不到兼顾
    title varchar(50) comment '博客主题',
    content text comment '博客正文',
    tag_id int comment '所属的标签号码',
    create_time varchar(50) comment '博客创建时间'
);

-- 标签表
drop table if exists tag_table;
create table tag_table
(
    tag_id int not null primary key auto_increment comment '标签号码',
    tag_name varchar(50) comment '标签名称'
);

drop table if exists user_table;
create table user_table(
  user_id int not null  primary key auto_increment comment '用户特定的id',
  user_name varchar(32) comment '用户名称',
  user_password varchar(32)  comment '用户密码'
);
