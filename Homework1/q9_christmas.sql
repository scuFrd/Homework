with product_plus(productid, productname) as 
(select id, productname
from product),
orderdetail_plus(orderid, productid) as
(select orderid, productid
from orderdetail),
order_plus(orderid, customerid, orderdate) as
(select id, customerid, orderdate
from 'order'),
customer_plus(customerid, companyname) as
(select id, companyname
from customer)
select group_concat(productname)
from(
select productname
from product_plus natural join orderdetail_plus natural join order_plus natural join customer_plus
where companyname = 'Queen Cozinha' and orderdate like '%2014-12-25%'
order by productid)
;
