#pragma once
#include<iostream>
#include<cstdio>
#include<mysql/mysql.h>
#include<cstring>
#include<memory>
#include<jsoncpp/json/json.h>


namespace blog_system{
    static MYSQL* MySQLInit(){
        //初始化一个MYSQL句柄并建立连接
        //1.创建一个句柄
        MYSQL* connectd_fd = mysql_init(NULL);
        //2.建立连接
        if(mysql_real_connect(connectd_fd,"127.0.0.1","root","0","blog_system",3306,NULL,0)==NULL)
        {
            printf("连接失败原因:%s\n",mysql_error(connectd_fd));
            return NULL;
        }

        //3.设定字符编码
        mysql_set_character_set(connectd_fd,"utf8");
        return connectd_fd;
    }

    static void MySQLRelease(MYSQL* connectd_fd){
        //释放句柄并点开链接
        mysql_close(connectd_fd);
    }


    //创建一个类
    class Blog_Table{
        public:
        Blog_Table(MYSQL* connectd_fd)
        :mysql_(connectd_fd)
        {
            //通过构造函数获取到一个 数据库 的操作句柄
        }

        //以下操作相关的参数都统一使用Json
        //Json::Value 就表示一个具体的json对象
        /*
            形如:
            {
                title:"博客正题",
                connect:"博客内容",
                create_time:"创建时间",
                tag_id:"标签id"
            }
        */
        //最大好处就是方便变更
        bool Insert(const Json::Value& blog){
            const std::string& content = blog["content"].asCString();

            std::unique_ptr<char> ptr(new char[content.size()*2+1]);

            mysql_real_escape_string(mysql_,ptr.get(),content.c_str(),content.size());
            //使用该函数将博客中的语句进行转义,确保文章的完整性
            //拼装sql语句
            std::unique_ptr<char> sql(new char[content.size()*2 + 4096]);
            sprintf(sql.get(),"insert into blog_table values(null,'%s','%s',%d,'%s')",
                blog["title"].asCString(),ptr.get(),
                blog["tag_id"].asInt(),
                blog["create_time"].asCString());
                //拼接sql指令

            int ret = mysql_query(mysql_,sql.get());
            //执行被拼接起来的sql指令
            if(ret != 0)
            {
              printf("插入博客失败%s\n",mysql_error(mysql_));
              return false;
            }
            printf("执行插入博客成功!\n");
            return true;
        }

        bool SelectAll(Json::Value* blogs,const std::string& tag_id =""){
            
            char sql[1024*4]={0};
            if( tag_id == "")
            {
                //不需要按照某个指定的标签来筛选博客
                sprintf(sql,"select blog_id,title,tag_id,create_time from blog_table");
            }
            else
            {  
                sprintf(sql,"select blog_id,title,tag_id,create_time from blog_table where tag_id = %d", std::stoi(tag_id));
            } 
            
            int ret = mysql_query(mysql_,sql);
            
            if(ret != 0)
            {
              printf("查询所有博客失败%s\n",mysql_error(mysql_));
              return false;
            }


            MYSQL_RES* result = mysql_store_result(mysql_);
            int rows = mysql_num_rows(result);
            
            for(int i=0;i<rows ;++i)
            {
              MYSQL_ROW row  = mysql_fetch_row(result);

              Json::Value blog;

              blog["blog_id"] = atoi(row[0]);
              //atoi 将C风格的字符串转换成数字
              blog["title"] = row[1];
              blog["tag_id"] =  atoi(row[2]);
              blog["create_time"] = row[3];
              blogs->append(blog);
            }

            mysql_free_result(result);
            printf("执行查找所有博客成功! 一共查找到%d 条博客!\n",rows);
            return true;
        }

        bool SelectOne(int32_t blog_id, Json::Value* blog){
          
          char sql[1024*4]={0};
          sprintf(sql,"select blog_id,title,content,tag_id,create_time from blog_table where blog_id = %d",blog_id);
          
          int ret = mysql_query(mysql_,sql);

          if(ret != 0){
            printf("查询单个博客失败!%s\n",mysql_error(mysql_));
            return false;
          }
          
          MYSQL_RES* result = mysql_store_result(mysql_);
          int  rows = mysql_num_rows(result);
          if( rows != 1 )
          {
            printf("查找到的博客不是只有一条,一共有%d条\n",rows);
            return false;
          }

          MYSQL_ROW row = mysql_fetch_row(result);
          (*blog)["blog_id"] =  atoi (row[0]);
          (*blog)["title"] = row[1];
          (*blog)["content"] = row[2];
          (*blog)["tag_id"] = atoi(row[3]);
          (*blog)["create_time"] = row[4];
          printf("执行查找单个博客成功!\n");
          return true;
        }

        bool Update(const Json::Value& blog){
            
            const std::string& content = blog["content"].asCString();
            //将修改博客的正文进行转义
            std::unique_ptr<char> ptr(new char[content.size()*2 + 1]);
            mysql_real_escape_string(mysql_,ptr.get(),content.c_str(),content.size());

            
            //核心就是拼接sql语句
            std::unique_ptr<char> sql(new char[content.size()*2 + 4096]);
            sprintf(sql.get(),"update blog_table set title = '%s',content='%s',tag_id = %d,create_time='%s' where blog_id = %d",
                blog["title"].asCString(),ptr.get(),blog["tag_id"].asInt(),blog["create_time"].asCString(),blog["blog_id"].asInt());

            int ret = mysql_query(mysql_,sql.get());

            if(ret != 0)
            {
              printf("更新指定博客失败!%s\n",mysql_error(mysql_));
              return false;
            }

            printf("更新博客成功!\n");
            return true;
        }

          bool Delete(int32_t blog_id){

            char sql[1024*4]={0};
            sprintf(sql,"delete from blog_table where blog_id=%d",blog_id);

            int ret = mysql_query(mysql_,sql);

            if(ret != 0)
            {
              printf("删除博客失败!%s\n",mysql_error(mysql_));
              return false;
            }

            printf("删除指定博客成功!\n");
            return true;
        }
        private:
            MYSQL* mysql_;
    };

    class Tag_Table{
      public:
        Tag_Table(MYSQL* mysql)
          :mysql_(mysql)
        {

        }

        bool Insert(const Json::Value& tag){
          char sql[1024*4]={0};
          sprintf(sql,"insert into tag_table values(null,'%s')",tag["tag_name"].asCString());

          int ret = mysql_query(mysql_,sql);

          if(ret !=0 )
          {
            printf("插入标签失败%s\n",mysql_error(mysql_));
            return false;
          }

          printf("插入标签成功!\n");
          return true;
        }

        bool Delete(int32_t tag_id){
          char sql[1024*4]={0};
          sprintf(sql,"delete from tag_table where tag_id=%d",tag_id);

          int ret = mysql_query(mysql_,sql);
          if(ret != 0)
          {
            printf("删除标签失败!%s\n",mysql_error(mysql_));
            return false;
          }

          printf("删除标签成功!\n");
          return true;
        }

        bool SelectAll(Json::Value* tags){
          char sql[1024*4]={0};
          sprintf(sql,"select tag_id,tag_name from tag_table");

          int ret = mysql_query(mysql_,sql);
          if(ret != 0)
          {
            printf("查找所有标签失败!%s\n",mysql_error(mysql_));
            return false;
          }

          MYSQL_RES* result = mysql_store_result(mysql_);

          int rows = mysql_num_rows(result);
          //获取查找结果的行数
          
          for(int i=0;i<rows;++i)
          {
            MYSQL_ROW row = mysql_fetch_row(result);
            Json::Value tag;
            tag["tag_id"] = atoi(row[0]);
            tag["tag_name"] = row[1];
            tags->append(tag);
          }

          printf("查看所有标签成功!\n");
          return true;
        }
      private:
        MYSQL* mysql_;
    };
}//end of blog_system