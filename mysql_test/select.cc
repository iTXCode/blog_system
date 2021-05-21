#include<cstdio>
#include<cstdlib>
#include<mysql/mysql.h>


int main(){
  //1.创建数据库句柄
  MYSQL* connectd_fd = mysql_init(NULL);
  //2.创建连接
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

  //4.拼接sql语句
  
  char sql[1024*4] = "select * from blog_table";

  //5.执行sql语句
  int ret = mysql_query(connectd_fd,sql);

  if(ret < 0)
  {
    printf("连接失败!%s\n",mysql_error(connectd_fd));
    mysql_close(connectd_fd);
    return 1;
  }

  //6.遍历结果集合,MYSQL_RES select得到的结果集合
  MYSQL_RES* result = mysql_store_result(connectd_fd);
  //   1.获取到结果集合中的行数和列数
  int rows = mysql_num_rows(result);
  int fields = mysql_num_fields(result);
  
  //2.根据行数和列数来遍历结果
  
  for(int i=0;i<rows;i++){
    MYSQL_ROW row = mysql_fetch_row(result);
    for(int j=0;j<fields;++j)
    {
      printf("%s\t",row[j]);
    }
    printf("\n");
  }

  mysql_free_result(result);
  mysql_close(connectd_fd);
  return 0;
}
