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
                sprintf(sql,"select blog_id,title,tag_id,create_time from blog_table where tag_id = %d limit 3", std::stoi(tag_id));
                printf("显示tag_id = %d 的博客",std::stoi(tag_id));
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

          bool SelectAKind(Json::Value* blogs,const std::string& tag_id ="1"){
            
            char sql[1024*4]={0};
            
           
            sprintf(sql,"select blog_id,title,tag_id,create_time from blog_table where tag_id = %d", std::stoi(tag_id));
            printf("显示tag_id = %d 的博客",std::stoi(tag_id));
            
            
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
          mysql_free_result(result);
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

        //修改某个标签
        bool Update_tag(int32_t tag_id, const Json::Value& tag){
            
           char sql[1024*4]={0};
            //将修改博客的正文进行转义
            
            
            //核心就是拼接sql语句
          
            sprintf(sql,"update tag_table set tag_name='%s' where tag_id = %d",tag["tag_name"].asCString(),tag_id);

            int ret = mysql_query(mysql_,sql);

            if(ret != 0)
            {
              printf("更新指定标签失败!%s\n",mysql_error(mysql_));
              return false;
            }

            printf("更新标签成功!\n");
            return true;
        }


        //查看某个标签
        bool SelectOne(int32_t tag_id,Json::Value* tag){
            char sql[1024*4]={0};
            sprintf(sql,"select tag_id,tag_name from tag_table where tag_id=%d",tag_id);

            int ret = 0;
            ret =  mysql_query(mysql_,sql);

            if(ret != 0){
              printf("查询单个标签失败!\n",mysql_error(mysql_));
              return false;
            }

            MYSQL_RES * result =mysql_store_result(mysql_);

            MYSQL_ROW row = mysql_fetch_row(result);
            (*tag)["tag_id"] = atoi(row[0]);
            (*tag)["tag_name"] =  row[1];
             mysql_free_result(result);

            printf("查找某个标签成功!");
            return true;
        }

        bool SelectAll(Json::Value* tags){
          char sql[1024*4]={0};
          sprintf(sql,"select tag_id,tag_name from tag_table");

          int ret = 0;
          ret =  mysql_query(mysql_,sql);
          if(ret != 0)
          {
            printf("查找所有标签失败!%s\n",mysql_error(mysql_));
            return false;
          }

          MYSQL_RES* result = mysql_store_result(mysql_);
          int rows = 0 ;
          if(result!=nullptr)
             rows = mysql_num_rows(result);
          //获取查找结果的行数
          
          for(int i=0;i<rows;++i)
          {
            MYSQL_ROW row = mysql_fetch_row(result);
            Json::Value tag;
            tag["tag_id"] = atoi(row[0]);
            tag["tag_name"] = row[1];
            tags->append(tag);
          }
          mysql_free_result(result);    
          printf("查看所有标签成功!\n");
          return true;
        }
      private:
        MYSQL* mysql_;
    };

    class User_table{
      public:

        User_table(MYSQL* mysql)
        :mysql_(mysql)
        {

        }
        bool Insert(const Json::Value& user_info){
            char sql[1024*4];
            sprintf(sql,"insert into user_table values(null,'%s','%s')",user_info["user_name"].asCString(),user_info["user_password"].asCString());

            int ret = mysql_query(mysql_,sql);

            if( ret != 0)
            {
              printf("用户注册失败! %s\n",mysql_error(mysql_));
              return false;
            }

            MYSQL_RES* result = mysql_store_result(mysql_);
            mysql_free_result(result);   
            printf("用户注册成功!\n");
            return true;
        }

        bool SelectOne(const Json::Value& user_info){
          //用户登录
          
          char sql[1024*4];
          sprintf(sql,"select user_name,user_password from user_table where user_name='%s' and user_password ='%s'",user_info["user_name"].asCString(),user_info["user_password"].asCString());
         
          int ret = mysql_query(mysql_,sql);
         
          
          if(!ret){
            printf("没有该用户%s\n",mysql_error(mysql_));
            return false;
          }
          MYSQL_RES* result = mysql_store_result(mysql_);
          mysql_free_result(result);    
          printf("用户登录成功\n");
          return true;
        }



      private:
      MYSQL* mysql_;
    };

     class Comment_Table{
      public:
        Comment_Table(MYSQL* mysql)
          :mysql_(mysql)
        {

        }

        bool Insert(const Json::Value& comment){
          char sql[1024*4]={0};
          sprintf(sql,"insert into comment_table values(null,%d,'%s','%s')",comment["blog_id"].asInt(),
          comment["comment"].asCString(),comment["comment_time"].asCString());

          int ret = mysql_query(mysql_,sql);

          if(ret !=0 )
          {
            printf("插入评论失败%s\n",mysql_error(mysql_));
            return false;
          }
          

          printf("插入评论成功!\n");
          return true;
        }

        bool Delete(int32_t comment_id){
          char sql[1024*4]={0};
          sprintf(sql,"delete from comment_table where comment_id=%d",comment_id);

          int ret = mysql_query(mysql_,sql);
          if(ret != 0)
          {
            printf("删除评论失败!%s\n",mysql_error(mysql_));
            return false;
          }

          printf("删除评论成功!\n");
          return true;
        }

        //查看某个标签
        bool SelectOne(int32_t blog_id,Json::Value* comments){
            char sql[1024*4]={0};
          
            sprintf(sql,"select comment_id,comment,comment_time from comment_table where blog_id = %d ", blog_id);
             printf("显示blog_id = %d 的博客",blog_id);
          
            
            int ret = 0;
            ret = mysql_query(mysql_,sql);
            
            if(ret != 0)
            {
              printf("查询所有评论失败%s\n",mysql_error(mysql_));
              return false;
            }


            MYSQL_RES* result = mysql_store_result(mysql_);
            if(result==nullptr){
              printf("该博客没有任何的评论记录!\n");
              return false;
            }

            int rows = mysql_num_rows(result);
            
            for(int i=0;i<rows ;++i)
            {
              MYSQL_ROW row  = mysql_fetch_row(result);

              Json::Value Comment;

              Comment["comment_id"] = atoi(row[0]);
              //atoi 将C风格的字符串转换成数字
              Comment["comment"] =  row[1];
              Comment["comment_time"] = row[2];
              comments->append(Comment);
            }

            mysql_free_result(result);
            printf("执行查找所有博客成功! 一共查找到%d 条博客!\n",rows);
            return true;
        }

          bool SelectAll(Json::Value* comments){
            
            char sql[1024*4]={0};
          
            sprintf(sql,"select comment_id,comment,comment_time from comment_table");
          
            
            int ret = 0;
            ret = mysql_query(mysql_,sql);
            
            if(ret != 0)
            {
              printf("查询所有评论失败%s\n",mysql_error(mysql_));
              return false;
            }


            MYSQL_RES* result = mysql_store_result(mysql_);
            if(result==nullptr){
              printf("没有任何的评论记录!\n");
              return false;
            }

            int rows = mysql_num_rows(result);
            
            for(int i=0;i<rows ;++i)
            {
              MYSQL_ROW row  = mysql_fetch_row(result);

              Json::Value Comment;

              Comment["comment_id"] = atoi(row[0]);
              //atoi 将C风格的字符串转换成数字
              Comment["comment"] =  row[1];
              Comment["comment_time"] = row[2];
             comments->append(Comment);
            }

            mysql_free_result(result);
            printf("执行查找所有博客成功! 一共查找到%d 条博客!\n",rows);
            return true;
        }

      
      private:
        MYSQL* mysql_;
    };


//用来实现留言功能

class Message_Table{
      public:
        Message_Table(MYSQL* mysql)
          :mysql_(mysql)
        {

        }

        bool Insert(const Json::Value& message){
          char sql[1024*4]={0};
          sprintf(sql,"insert into message_table values(null,'%s','%s')",
          message["message_content"].asCString(),message["message_time"].asCString());

          int ret = mysql_query(mysql_,sql);

          if(ret !=0 )
          {
            printf("插入留言失败%s\n",mysql_error(mysql_));
            return false;
          }

          printf("插入留言成功!\n");
          return true;
        }

        bool Delete(int32_t message_id){
          char sql[1024*4]={0};
          sprintf(sql,"delete from message_table where message_id=%d",message_id);

          int ret = mysql_query(mysql_,sql);
          if(ret != 0)
          {
            printf("删除评论失败!%s\n",mysql_error(mysql_));
            return false;
          }

          printf("删除评论成功!\n");
          return true;
        }

        //查看某个标签
        bool SelectOne(int32_t message_id,Json::Value* messages){
            char sql[1024*4]={0};
          
            sprintf(sql,"select message_id,message_content,message_time from message_table where message_id = %d ", message_id);
          
            
            int ret = 0;
            ret = mysql_query(mysql_,sql);
            
            if(ret != 0)
            {
              printf("查询某个留言失败%s\n",mysql_error(mysql_));
              return false;
            }


            MYSQL_RES* result = mysql_store_result(mysql_);
            if(result==nullptr){
              printf("没有任何的留言!\n");
              return false;
            }

              Json::Value Message;
              MYSQL_ROW row  = mysql_fetch_row(result);
              Message["message_id"] = atoi(row[0]);
              //atoi 将C风格的字符串转换成数字
              Message["message_content"] =  row[1];
              Message["message_time"] = row[2];
              messages->append(Message);
            

            mysql_free_result(result);
           printf("查询某条留言成功\n");
            return true;
        }

          bool SelectAll(Json::Value* messages){
            
            char sql[1024*4]={0};
          
            sprintf(sql,"select message_id,message_content,message_time from message_table");
          
            
            int ret = 0;
            ret = mysql_query(mysql_,sql);
            
            if(ret != 0)
            {
              printf("查询所有留言失败%s\n",mysql_error(mysql_));
              return false;
            }


            MYSQL_RES* result = mysql_store_result(mysql_);
            if(result==nullptr){
              printf("没有任何的留言记录!\n");
              return false;
            }

            int rows = mysql_num_rows(result);
            
            for(int i=0;i<rows ;++i)
            {
              MYSQL_ROW row  = mysql_fetch_row(result);

              Json::Value Message;

              Message["message_id"] = atoi(row[0]);
              //atoi 将C风格的字符串转换成数字
              Message["message_content"] =  row[1];
              Message["message_time"] = row[2];
              messages->append(Message);
            }

            mysql_free_result(result);
            printf("执行查找所有留言成功! 一共查找到%d 条留言!\n",rows);
            return true;
        }

      
      private:
        MYSQL* mysql_;
    };
}//end of blog_system
