with product_plus(productid, productname, discontinued) as
(select id, productname, discontinued
from product where discontinued = 1),
orderdetail_plus(orderid, productid) as
(select orderid, productid
from orderdetail),
order_plus(orderid, customerid, orderdate) as
(select id, customerid, orderdate
from 'order'),
customer_plus(customerid, companyname, contactname) as
(select id, companyname, contactname
from customer)
select productname, companyname, contactname
from product_plus natural join orderdetail_plus natural join order_plus natural join customer_plus
group by productname
having min(orderdate)
;

