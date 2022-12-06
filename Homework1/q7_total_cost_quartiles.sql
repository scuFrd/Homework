with orderdetail_plus(orderid, unitprice, quantity) as
(select orderid, unitprice, quantity
from orderdetail),
order_plus(orderid, customerid) as
(select id, customerid
from 'order'),
customer_plus(customerid, companyname) as 
(select id, companyname
from customer)
select companyname, customerid, totalexpenditures
from(
select *,ntile(4) over (order by totalexpenditures) ntile
from(
select ifnull(companyname,'MISSING_NAME') as companyname,customerid,round(sum(unitprice*quantity),2) as totalexpenditures
from order_plus natural left outer join customer_plus natural join orderdetail_plus
group by customerid))
where ntile = 1
;


