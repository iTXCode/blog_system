#include<regex>
#include<string>
#include<signal.h>
#include"db.hpp"
#include"httplib.h"


MYSQL* mysql=NULL;

char tmp[64];

std::string getTime()
{
  time_t timep;
  time (&timep);
  strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S",localtime(&timep) );
  return tmp;
}


int main(){
  using namespace httplib;
  using namespace blog_system;
  //1.先和数据库建立连接
  mysql = blog_system::MySQLInit();
  signal(SIGINT,[](int){blog_system::MySQLRelease(mysql);
      exit(0);});
  //2.创建相关数据库处理对象
  Blog_Table blog_table(mysql);
  Tag_Table tag_table(mysql);
  User_table user_table(mysql);
  Comment_Table comment_table(mysql);
  Message_Table message_table(mysql);

  //3.创建服务器,并设置"路由"
  //HTTP服务器中的路由,和IP协议的路由不一样
  //此处的路由指的是把 方法+path =>那个处理函数 关联关系声明清楚
  Server server;

  //新增博客
  server.Post("/blog",[&blog_table](const Request& req,
        Response& resp){
       printf("新增博客!\n");

       //1.获取到请求中的body并解析成json格式
       Json::Reader reader;
       Json::Value req_json;
       Json::Value resp_json;
       Json::FastWriter writer;
       bool ret = reader.parse(req.body,req_json);
       //完成将body(字符串)转换成json格式

       if(!ret)
       {
         //解析失败
         printf("解析请求格式失败!%s\n",req.body.c_str());
         //构造一个响应对象,告诉客户端出错了
         resp_json["ok"] = false;
         resp_json["reason"] = "input data parse  error!";
         resp.status = 400;
         resp.set_content(writer.write(resp_json),"application/json");
         //将出错原因发送给用户,其中的application/json 为http的固定格式
         return ;
       }//end of if 
        
       //2.将解析之后的json格式中的内容进行校验
       if(req_json["title"].empty() 
           || req_json["content"].empty()
           || req_json["tag_id"].empty())
       {
         printf("请求数据格式有误!%s\n",req.body.c_str());
         //构造一个响应对象,告诉客户端出错了
         resp_json["ok"] =  false;
         resp_json["reason"] = "input data format error!";
         resp.status = 400;
         resp.set_content(writer.write(resp_json),"application/json");
         return ;
       }

       //3.真正的调用MySQL接口来操作
       std::string C_Time = getTime();
       req_json["create_time"] = C_Time.c_str();
       ret = blog_table.Insert(req_json);

       if(!ret)
       {
         //插入博客失败
         printf("插入博客失败!\n");
         resp_json["ok"] = false;
         resp_json["reason"] = "blog insert failed!";
         resp.status = 500;
         resp.set_content(writer.write(resp_json),"application/json");
         return;
       }

       //4.构造一个正确的响应给客户端即可
       printf("博客插入成功!\n");
       resp_json["ok"] = true;
       resp.set_content(writer.write(resp_json),"application/json");
       return;
      });

  //查看所有博客
  server.Get("/blog",[&blog_table](const Request& req,Response& resp){
        
      printf("查看所有博客!\n");
      //1.尝试获取 tag_id ,如果 tag_id 这个参数不存在,
      //  返回空字符串
      const std::string& tag_id =  req.get_param_value("tag_id");
 
      // 获取解析结果中的tag_id
      //都不需解析请求的时候,也就不需要合法判定了
      //2.调用数据库操作来获取所有博客结果
       Json::FastWriter writer;
       Json::Value resp_json; 
      
      bool ret = blog_table.SelectAll(&resp_json,tag_id);

      if(!ret)
      {
        resp_json["ok"] = false;
        resp_json["reason"] = "select all failed!";
        resp.status = 500;
        resp.set_content(writer.write(resp_json),"application/json");
        return ;
      }
      
      //3.查询成功了就构造一个成功的结果返回
      resp.status=200;
      resp.set_content(writer.write(resp_json),"application/json");
      return ;
      });



  //查看某个博客
  //原始字符串R("(/blog/(\d+))")
  //(/blog/(\\d+))其中的\\出现是为了转义"\"
  
  server.Get(R"(/blog/(\d+))",[&blog_table](const Request& req,Response& resp){
      //1.解析过去到 blog_id 
      int32_t blog_id = std::stoi(req.matches[1].str());
      printf("查看id 为%d 的博客!\n",blog_id);

      //2.直接调用数据库操作

      Json::Value resp_json;
      Json::FastWriter writer;
      bool ret = blog_table.SelectOne(blog_id,&resp_json);  
      
      if(!ret)
      {  //这里是指调用数据库操作失败
        resp_json["ok"] = false;
        resp_json["reason"] = "select one failed!" + std::to_string(blog_id);
        resp.status = 404;
        resp.set_content(writer.write(resp_json),"application/json");
        return ;
      }

     	//3.包装一个正确的返回结果
      resp.set_content(writer.write(resp_json),"application/json");
      return;
      });

    server.Get(R"(/blogs/(\d+))",[&blog_table](const Request& req,Response& resp){
      //1.解析过去到 blog_id 
      const std::string&  tag_id = req.matches[1].str();

      //2.直接调用数据库操作

      Json::Value resp_json;
      Json::FastWriter writer;
      bool ret = blog_table.SelectAKind(&resp_json,tag_id);

      if(!ret)
      {  //这里是指调用数据库操作失败
      printf("查询失败\n");
        resp_json["ok"] = false;
        resp_json["reason"] = "select one kind failed!";
        resp.status = 404;
        resp.set_content(writer.write(resp_json),"application/json");
        return ;
      }

      //3.包装一个正确的返回结果
      resp.set_content(writer.write(resp_json),"application/json");
      return;
      });

  //修改某个博客
       server.Put(R"(/blog/(\d+))",[&blog_table](const Request& req,Response& resp){
            //1.先获取需要修改的博客id 
            int32_t blog_id = std::stoi(req.matches[1].str());
            printf("修改 id 为 %d 的博客!\n",blog_id);

            //2.获取到请求并解析结果
            Json::Reader reader;
            Json::FastWriter writer;
            Json::Value req_json;
            Json::Value resp_json;

            bool ret = reader.parse(req.body,req_json);

            if(!ret)
            {
              resp_json["ok"] = false;
              resp_json["reason"] = "解析请求失败!\n";
              resp.status = 400;
              resp.set_content(writer.write(resp_json),"application/json");
              return ;
            }

            //3.校验参数是否符合预期
            if(req_json["title"].empty()
                || req_json["content"].empty()
                || req_json["tag_id"].empty())
            {
              resp_json["ok"] = false;
              resp_json["reason"] = "update blog parse request failed!";
              resp.status = 400;
              resp.set_content(writer.write(resp_json),"application/json");
              return ;
            }

            //4.调用数据库操作完成更新博客操作
            req_json["blog_id"] = blog_id;//从path 中得到的 id 设置到 json 对象中
            std::string C_Time = getTime();
            req_json["create_time"] = C_Time.c_str();
            ret = blog_table.Update(req_json);

            if(!ret)
            {
              //数据库操作失败
              resp_json["ok"] = false;
              resp_json["reason"] = "update blog database failed!";
              resp.status = 500;
              resp.set_content(writer.write(resp_json),"application/json");
              return ;
            }

            // 5 构造正确的响应结果
            resp_json["ok"] = true;
            resp.set_content(writer.write( resp_json ),"application/json");
            return ;

        });

  //删除
    server.Delete(R"(/blog/(\d+))",[&blog_table](const Request& req,Response& resp){
            //获取博客id
            Json::FastWriter writer;
            Json::Value resp_json;
            int32_t blog_id =std::stoi(req.matches[1].str()) ;
            printf("删除id为%d的博客!\n",blog_id);
            
            //2.调用数据库操作
            bool ret = blog_table.Delete(blog_id);
            if(!ret){
              resp_json["ok"] = false;
              resp_json["reason"] = "delete blog failed!";
              resp.status = 500;
              resp.set_content(writer.write(resp_json),"application/json");
              return;
            }

            //构造一个正确的返回结果
            resp_json["ok"] = true;
            resp.set_content(writer.write(resp_json),"application/json");
            return ;
        });


  //新增标签
    server.Post("/tag",[&tag_table](const Request& req,Response& resp){
          
          Json::FastWriter writer;
          Json::Reader reader;
          Json::Value resp_json;
          Json::Value req_json;
           //1.解析请求
         
          bool ret = reader.parse(req.body,req_json);
           
          if(!ret){
            printf("插入标签失败!\n");
            resp_json["ok"] =  false; 
            resp_json["reason"] = " insert tag req parse failed";
            resp.status = 500;
            resp.set_content(writer.write(resp_json),"application/josn");
            return ;
          }

          //2.对请求进行校验
          if(req_json["tag_name"].empty())
          {
            printf("校验过程有误!\n");
            resp_json["ok"] = false;
            resp_json["reason"] = "insert tag req format error";
            resp.status = 400;
            resp.set_content(writer.write(resp_json),"application/json");
            return ;
          }

          //3.调用数据库操作函数来实现该功能
           ret  = tag_table.Insert(req_json);

           if(!ret)
           {
              printf("数据库操作错误!\n");
              resp_json["ok"] = false;
              resp_json["reason"] = "insert tag failed";
              resp.status = 400;
              resp.set_content(writer.write(resp_json),"application/json");
              return ;
           }

          //构造一个正确的响应结果
          resp_json["ok"] = true;
          resp.set_content(writer.write(resp_json),"application/json");
          return ;
        });

  //删除标签
    server.Delete(R"(/tag/(\d+))",[&tag_table](const Request& req,Response& resp){

            Json::FastWriter writer;
            Json::Value resp_json;

            //1.获取要删除的标签的id
            int32_t tag_id = std::stoi(req.matches[1].str());
            printf("要删除的标签的id=%d\n",tag_id);

            //2.调用数据库操作来完成删除指定标签的操作
            bool ret = tag_table.Delete(tag_id);

            if(!ret)
            {
              printf("删除标签失败!\n");
              resp_json["ok"] = false;
              resp_json["reason"] = "delete tag failed!";
              resp.status = 500;
              resp.set_content(writer.write(resp_json),"application/json");
              return ;
            }

            //3.构造正确的返回结果
            resp_json["ok"] = true;
            resp.set_content(writer.write(resp_json),"application/json");
            return;
            
        });

//修改某个标签名字
  server.Put(R"(/tag/(\d+))",[&tag_table](const Request& req,Response& resp){
            //1.先获取需要修改的博客id 
            int32_t tag_id = std::stoi(req.matches[1].str());
            printf("修改 tag 为 %d 的标签!\n",tag_id);

            //2.获取到请求并解析结果
            Json::Reader reader;
            Json::FastWriter writer;
            Json::Value req_json;
            Json::Value resp_json;

            bool ret = reader.parse(req.body,req_json);

            if(!ret)
            {
              resp_json["ok"] = false;
              resp_json["reason"] = "标签修改解析请求失败!\n";
              resp.status = 400;
              resp.set_content(writer.write(resp_json),"application/json");
              return ;
            }

            //3.校验参数是否符合预期
            if( req_json["tag_name"].empty())
            {
              resp_json["ok"] = false;
              resp_json["reason"] = "update tag  request failed!";
              resp.status = 400;
              resp.set_content(writer.write(resp_json),"application/json");
              return ;
            }

            //4.调用数据库操作完成更新博客操作
            ret = tag_table.Update_tag(tag_id,req_json);

            if(!ret)
            {
              //数据库操作失败
              resp_json["ok"] = false;
              resp_json["reason"] = "update tag database failed!";
              resp.status = 500;
              resp.set_content(writer.write(resp_json),"application/json");
              return ;
            }

            // 5 构造正确的响应结果
            resp_json["ok"] = true;
            resp.set_content(writer.write( resp_json ),"application/json");
            return ;

        });

  //查看所有标签
   server.Get("/tag",[&tag_table](const Request& req,Response& resp){
           
         Json::Value resp_json;
         Json::FastWriter writer;
         //1.不需要参数,直接执行数据库
       
         bool ret= tag_table.SelectAll(&resp_json);
         if(!ret)
         {
             printf("查看所有标签失败!\n");
             resp_json["ok"] = false;
             resp_json["reason"] = "select all failed!";
             resp.status = 500;
             resp.set_content(writer.write(resp_json),"application/json");
             return ;
         }

         //构造正确的返回结果
         resp.status = 200;
         resp.set_content(writer.write(resp_json),"application/json");
         printf("成功\n");
       });



        server.Get(R"(/tag/(\d+))",[&tag_table](const Request& req,Response& resp){
          //1.解析获得的tag_id

          int32_t tag_id = std::stoi(req.matches[1].str());
          printf("查询 tag_id = %d 的标签\n",tag_id);

          //2.直接条用数据库操作
          Json::Value resp_json;
          Json::FastWriter writer;
          bool ret = tag_table.SelectOne(tag_id,&resp_json);

          if(!ret){
            //这里是调用数据库操作失败
            resp_json["ok"] = false;
            resp_json["reason"] = "select one failed!";
            resp.status = 404;
            resp.set_content(writer.write(resp_json),"application/json");
            return;
          }

          //3.包装一个正确的返回结果
          resp.set_content(writer.write(resp_json),"application/json");
          return;
        });


       server.Post("/user",[&user_table](const Request& req,Response& resp){
         printf("用户注册!\n");
         Json::FastWriter writer;
         Json::Reader reader;
         Json::Value resp_json;
         Json::Value req_json;

         //1.解析请求

         bool  ret = reader.parse(req.body,req_json);

         if(!ret){
           printf("插入用户失败!\n");
           resp_json["ok"] = false;
           resp_json["reason"] ="insert user_info rep parse failed!";
           resp.status =500;
           return;
         }

         //2.对请求进行校验
         if(req_json["user_name"].empty() || req_json["user_password"].empty()){
           printf("校验过程有误!\n");
           resp_json["ok"] = false;
           resp_json["reason"] ="insert user req format error";
           resp.status = 400;
           resp.set_content(writer.write(resp_json),"application/json");
           return ;
         }

         //3.调用数据库操作函数来完成该功能实现

         ret = user_table.Insert(req_json);

         if(!ret){
           printf("该功能数据库操作有误!\n");
           resp_json["ok"] = false;
           resp_json["reason"] = "insert user failed!";
           resp.status =400;
           resp.set_content(writer.write(resp_json),"application/json");
           return;
         }

         //构造一个争取的返回结果
         resp_json["ok"] = true;
         resp.set_content(writer.write(resp_json),"application/json");
         return ;
       });

       server.Post("/users",[&user_table](const Request& req,Response& resp){
         

         printf("用户登录!\n");
         Json::FastWriter writer;
         Json::Reader reader;
         Json::Value resp_json;
         Json::Value req_json;

         //1.解析请求
         printf("hehe");

         bool  ret = reader.parse(req.body,req_json);

         if(!ret){
           printf("数据解析失败!\n");
           resp_json["ok"] = false;
           resp_json["reason"] ="login user_info rep parse failed!";
           resp.status =500;
           return;
         }

         //2.对请求进行校验
         if(req_json["user_name"].empty() || req_json["user_password"].empty()){
           printf("校验过程有误!\n");
           resp_json["ok"] = false;
           resp_json["reason"] ="login user req format error";
           resp.status = 400;
           resp.set_content(writer.write(resp_json),"application/json");
           return ;
         }

         //3.调用数据库操作函数来完成该功能实现

         ret = user_table.SelectOne(req_json);

         if(!ret){
           printf("该功能数据库操作有误!\n");
           resp_json["ok"] = false;
           resp_json["reason"] = "login user failed!";
           resp.status =400;
           resp.set_content(writer.write(resp_json),"application/json");
           return;
         }

         //构造一个正确的返回结果
         resp_json["ok"] = true;
         printf("hahahah");
         resp.set_content(writer.write(resp_json),"application/json");
         return ;
       });


         server.Post("/comments",[&comment_table](const Request& req,Response& resp){
         printf("插入评论!\n");
         
         Json::FastWriter writer;
         Json::Reader reader;
         Json::Value resp_json;
         Json::Value req_json;

         //1.解析请求

         bool  ret = reader.parse(req.body,req_json);

         if(!ret){
           printf("插入评论解析失败!\n");
           resp_json["ok"] = false;
           resp_json["reason"] ="insert user_info rep parse failed!";
           resp.status =500;
           return;
         }

         //2.对请求进行校验
         if(req_json["blog_id"].empty() || req_json["comment"].empty()){
           printf("校验过程有误!\n");
           resp_json["ok"] = false;
           resp_json["reason"] ="insert comment req format error";
           resp.status = 400;
           resp.set_content(writer.write(resp_json),"application/json");
           return ;
         }

         //3.调用数据库操作函数来完成该功能实现
        std::string C_Time = getTime();
        req_json["comment_time"] = C_Time.c_str();
         ret = comment_table.Insert(req_json);

         if(!ret){
           printf("该功能数据库操作有误!\n");
           resp_json["ok"] = false;
           resp_json["reason"] = "insert comment failed!";
           resp.status =400;
           resp.set_content(writer.write(resp_json),"application/json");
           return;
         }

         //构造一个争取的返回结果
         resp_json["ok"] = true;
         resp.set_content(writer.write(resp_json),"application/json");
         return ;
       }); 

        server.Get("/comments",[&comment_table](const Request& req,Response& resp){
          //1.解析获得的tag_id

          int32_t blog_id = std::stoi(req.matches[1].str());
          printf("查询 blog_id = %d 的评论\n",blog_id);

          //2.直接条用数据库操作
          Json::Value resp_json;
          Json::FastWriter writer;
          bool ret = comment_table.SelectOne(blog_id,&resp_json);

          if(!ret){
            //这里是调用数据库操作失败
            resp_json["ok"] = false;
            resp_json["reason"] = "select one failed!";
            resp.status = 404;
            resp.set_content(writer.write(resp_json),"application/json");
            return;
          }

          //3.包装一个正确的返回结果
          resp_json["ok"]=true;
          resp.set_content(writer.write(resp_json),"application/json");
          return;
        });


//查看所有评论
    server.Get("/comments",[&comment_table](const Request& req,Response& resp){
        
      printf("查看所有评论!\n");
      //1.尝试获取 tag_id ,如果 tag_id 这个参数不存在,
      //  返回空字符串
      // 获取解析结果中的tag_id
      //都不需解析请求的时候,也就不需要合法判定了
      //2.调用数据库操作来获取所有博客结果
       Json::FastWriter writer;
       Json::Value resp_json; 
      
      bool ret = comment_table.SelectAll(&resp_json);

      if(!ret)
      {
        resp_json["ok"] = false;
        resp_json["reason"] = "select all comment failed!";
        resp.status = 500;
        resp.set_content(writer.write(resp_json),"application/json");
        return ;
      }
      
      //3.查询成功了就构造一个成功的结果返回
      resp.status=200;
      resp_json["ok"] = true;
      resp.set_content(writer.write(resp_json),"application/json");
      return ;
      });



//留言板块

       server.Post("/message",[&message_table](const Request& req,Response& resp){
         printf("插入留言!\n");
         
         Json::FastWriter writer;
         Json::Reader reader;
         Json::Value resp_json;
         Json::Value req_json;

         //1.解析请求

         bool  ret = reader.parse(req.body,req_json);

         if(!ret){
           printf("插入留言解析失败!\n");
           resp_json["ok"] = false;
           resp_json["reason"] ="insert message rep parse failed!";
           resp.status =500;
           return;
         }

         //2.对请求进行校验
         if( req_json["message_content"].empty()){
           printf("校验过程有误!\n");
           resp_json["ok"] = false;
           resp_json["reason"] ="insert message null";
           resp.status = 400;
           resp.set_content(writer.write(resp_json),"application/json");
           return ;
         }

         //3.调用数据库操作函数来完成该功能实现
        std::string C_Time = getTime();
        req_json["message_time"] = C_Time.c_str();
      
         ret = message_table.Insert(req_json);

         if(!ret){
           printf("该功能数据库操作有误!\n");
           resp_json["ok"] = false;
           resp_json["reason"] = "insert message failed!";
           resp.status =400;
           resp.set_content(writer.write(resp_json),"application/json");
           return;
         }

         //构造一个争取的返回结果
         resp_json["ok"] = true;
         resp.set_content(writer.write(resp_json),"application/json");
         return ;
       }); 

//查看某个留言
        server.Get(R"(/message/(\d+))",[&message_table](const Request& req,Response& resp){
          //1.解析获得的tag_id

          int32_t message_id = std::stoi(req.matches[1].str());
          printf("查询 message_id = %d 的留言\n",message_id);

          //2.直接条用数据库操作
          Json::Value resp_json;
          Json::FastWriter writer;
          bool ret = message_table.SelectOne(message_id,&resp_json);

          if(!ret){
            //这里是调用数据库操作失败
            resp_json["ok"] = false;
            resp_json["reason"] = "select one message failed!";
            resp.status = 404;
            resp.set_content(writer.write(resp_json),"application/json");
            return;
          }

          //3.包装一个正确的返回结果
          resp_json["ok"]=true;
          resp.set_content(writer.write(resp_json),"application/json");
          return;
        });


//查看所有留言
    server.Get("/messages",[&message_table](const Request& req,Response& resp){
        
      printf("查看所有留言!\n");
      //1.尝试获取 tag_id ,如果 tag_id 这个参数不存在,
      //  返回空字符串
      // 获取解析结果中的tag_id
      //都不需解析请求的时候,也就不需要合法判定了
      //2.调用数据库操作来获取所有博客结果
       Json::FastWriter writer;
       Json::Value resp_json; 
      
      bool ret = message_table.SelectAll(&resp_json);
      if(!ret)
      {
        resp_json["ok"] = false;
        resp_json["reason"] = "select all message failed!";
        resp.status = 500;
        resp.set_content(writer.write(resp_json),"application/json");
        return ;
      }
    
      //3.查询成功了就构造一个成功的结果返回
      resp.status=200;
      resp.set_content(writer.write(resp_json),"application/json");
      return ;
      });


      server.Delete(R"(/message/(\d+))",[&message_table](const Request& req,Response& resp){
        int32_t message_id = std::stoi(req.matches[1].str());

          Json::Value resp_json;
          Json::FastWriter writer;
          bool ret = message_table.Delete(message_id);
          printf("hehe\n");

          if(!ret){
            //这里是调用数据库操作失败
            resp_json["ok"] = false;
            resp_json["reason"] = "select one message failed!";
            resp.status = 404;
            resp.set_content(writer.write(resp_json),"application/json");
            return;
          }


          //3.包装一个正确的返回结果
          printf("hehe\n");
          resp_json["ok"] = true;
          resp.set_content(writer.write(resp_json),"application/json");
          printf("hehe\n");
          return ;
      });

     
  server.set_base_dir("./xroot");
  server.listen("0.0.0.0",9093);
  return 0;
}
