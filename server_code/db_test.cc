#include"db.hpp"

void TestBlogTable(){
  Json::StyledWriter writer;//按照Json格式打印出博客
  MYSQL* mysql = blog_system::MySQLInit();
  blog_system::Blog_Table blog_table(mysql);
 
  Json::Value blog;
  bool ret = false;
//测试插入
// blog["title"]="毕设进行时!";
// blog["content"]="我今天进行后端代码的编辑!";
// blog["tag_id"]=1;
// blog["create_time"]="2021/5/17";
// bool ret = blog_table.Insert(blog);
// printf("insert:%d\n",ret);


//查找某个博客
// ret = blog_table.SelectOne(6,&blog);
// printf("select one:%d\n",ret);
// printf("blog:%s\n",writer.write(blog).c_str());
  
//测试更新某篇博客
//blog["blog_id"]=4;
//blog["title"]="毕设进行中!";
//blog["content"]="我今天进行后端代码的编辑!";
//
//ret = blog_table.Update(blog);
//printf("Update:%d\n",ret);
//printf("blog:%s\n",writer.write(blog).c_str());
//
// Json::Value blogs;
///测试查找所有博客
// ret= blog_table.SelectAll(&blogs);
// printf("SelectAll:%d\n",ret);
// printf("blogs:%s\n",writer.write(blogs).c_str());

 
  //测试删除博客
  ret = blog_table.Delete(2);
  printf("delete:%d\n",ret);

  
  blog_system::MySQLRelease(mysql);
}

void TestTagTable(){
  Json::StyledWriter writer;
  MYSQL* mysql = blog_system::MySQLInit();
  blog_system::Tag_Table tag_table(mysql);
  bool  ret = false;
  
 
  //测试添加标签
  Json::Value tag;
// tag["tag_id"]=2;
// tag["tag_name"]="java";
// ret = tag_table.Insert(tag);
// printf("insert:%d\n",ret);
// printf("tag:%s\n",writer.write(tag).c_str());

//测试删除标签
// ret = tag_table.Delete(2);
// printf("delete tag:%d\n",ret);
//
  //测试查找标签
  //Json::Value tags;
  //ret =  tag_table.SelectAll(&tags); 
  //printf("select all:%d\n",ret);
  //printf("tags:%s\n",writer.write(tags).c_str());
  
  blog_system::MySQLRelease(mysql);
}

int main(){
  //TestBlogTable();
  TestTagTable();
  return 0;
}
