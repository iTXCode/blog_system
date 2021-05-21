#include<cstdio>
#include<cstdlib>
//编辑器默认从 /usr/include 目录中查找头文件,mysql。h
//是在一个mysql的目录中
#include<mysql/mysql.h>


int main(){
  //1.创建一个数据库连接的句柄
  MYSQL* connectd_fd = mysql_init(NULL);//论文中需要对函数进行论述
  //2.和数据库建立连接，应用层之间的建立连接(与TCP连接有异)
  //连接过程需要指定一些必要的信息
  //    1.连接句柄
  //    2.服务器的ip地址
  //    3.服务器的端口号
  //    4.用户名
  //    5.密码
  //    6.数据库名(blog_system)
  //    7.unix_sock NULL 
  //    8.client_flag 0
  if(  mysql_real_connect(connectd_fd,"127.0.0.1","root","0","blog_system",3306,NULL,0) == NULL )
  {
    //数据库连接失败
    printf("连接失败!%s\n",mysql_error(connectd_fd));
    return 1;
  }

  printf("连接成功!\n");


  //3.设置编码方式
  //   mysql server部分最初安装的时候已经设置成了 utf8
  //   也得在客户端这边设置成utf8
  
  mysql_set_character_set(connectd_fd,"utf8");

  //4.连接SQL语句
  
  char sql[1024*4] = {0};
  char title[] = "写论文";
  char content[]="先做毕设";
  int tag_id = 1;
  char date[] = "2021/05/14";
  sprintf(sql,"insert into blog_table values(null,'%s','%s',%d,'%s')",
      title,content,tag_id,date);
  printf("sql:%s\n",sql); //打印出拼接的sql语句

  //5.让 数据库 服务器执行SQL
  
  int ret = mysql_query(connectd_fd,sql);

  if(ret<0)
  {
    printf("执行sql语句失败!%s\n",mysql_error(connectd_fd));
    mysql_close(connectd_fd);//关闭数据库句柄   
    return 1;
  }
  printf("插入成功!\n");
    
  sprintf(sql,"select * from blog_table");

  mysql_close(connectd_fd);//关闭数据库句柄
  return 0;
}
